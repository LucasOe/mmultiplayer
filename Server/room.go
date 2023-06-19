package main

import (
	"context"
	"net"
	"sync"
	"time"
)

type Room struct {
	Name           string
	rwMu           sync.RWMutex
	Clients        []*Client
	gameMode       string
	taggedPlayerId uint32
	canTag         bool
	cancelTagStart func()
	tagCoolDown    time.Duration
	cancelTagCheck func()
}

func (room *Room) AddPlayer(client *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.Clients = append(room.Clients, client)

	// Notify the other clients that a new client connected
	room.sendMessageExceptUnsafe(client.Id, map[string]interface{}{
		"type":      "connect",
		"id":        client.Id,
		"name":      client.name,
		"character": client.character,
		"level":     client.level,
	})

	// Tell the client the existing clients
	for _, c := range room.Clients {
		if c.Id != client.Id {
			go client.SendMessage(map[string]interface{}{
				"type":           "connect",
				"id":             c.Id,
				"name":           c.name,
				"character":      c.character,
				"level":          c.level,
				"taggedPlayerId": room.taggedPlayerId,
				"canTag":         room.canTag,
			})
		}
	}
}

func (room *Room) StartTagGameMode() {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.gameMode = TagGameMode

	room.sendMessageUnsafe(map[string]interface{}{
		"type":     "gameMode",
		"gameMode": TagGameMode,
	})

	room.tagRandomPlayerUnsafe()
}

func (room *Room) tagRandomPlayerUnsafe() {
	var randPlayerId uint32

	numPlayers := len(room.Clients)
	switch numPlayers {
	case 0:
		return
	case 1:
		randPlayerId = room.Clients[0].Id
	default:
		randPlayerId = room.Clients[system.rng.Intn(numPlayers)].Id
	}

	room.setTaggedPlayerUnsafe(randPlayerId)
}

func (room *Room) EndGameMode() {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.gameMode = ""

	room.taggedPlayerId = 0
	room.canTag = false
	if room.cancelTagStart != nil {
		room.cancelTagStart()
		room.cancelTagStart = nil
	}

	if room.cancelTagCheck != nil {
		room.cancelTagCheck()
		room.cancelTagCheck = nil
	}

	room.sendMessageUnsafe(map[string]interface{}{
		"type":     "gameMode",
		"gameMode": "",
	})
}

func (room *Room) SetTagCooldown(coolDown time.Duration) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.tagCoolDown = coolDown
}

func (room *Room) CurrentTaggedPlayerId() uint32 {
	room.rwMu.RLock()
	defer room.rwMu.RUnlock()

	return room.taggedPlayerId
}

func (room *Room) IsTaggingEnabled() bool {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	return room.canTag
}

func (room *Room) OnPlayerDisconnect(disconnectedPlayer *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if room.Name == "" {
		// If room name is empty string it is a placeholder room
		return
	}

	var newClients []*Client

	for _, c := range room.Clients {
		if c.Id != disconnectedPlayer.Id {
			newClients = append(newClients, c)
		}
	}

	room.Clients = newClients
	if len(room.Clients) == 0 {
		system.RemoveRoom(room.Name)
	}

	room.removePlayerUnsafe(disconnectedPlayer)
}

func (room *Room) removePlayerUnsafe(player *Client) {
	room.sendMessageExceptUnsafe(player.Id, map[string]interface{}{
		"type": "disconnect",
		"id":   player.Id,
	})

	if room.gameMode == TagGameMode && player.Id == room.taggedPlayerId {
		if room.cancelTagStart != nil {
			room.cancelTagStart()
		}

		room.tagRandomPlayerUnsafe()
	}
}

func (room *Room) EnableCanTag() {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.canTag = true

	room.sendMessageUnsafe(map[string]interface{}{
		"type": "canTag",
	})

	if room.cancelTagCheck != nil {
		room.cancelTagCheck()
	}

	ctx, cancelFn := context.WithCancel(context.Background())
	room.cancelTagCheck = cancelFn

	go room.newPlayerTaggedLoop(ctx)
}

func (room *Room) PlayerDied(player *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if !room.canTag {
		return
	}

	var isClientInRoom bool
	for _, client := range room.Clients {
		if client.Id == room.taggedPlayerId {
			isClientInRoom = true
			break
		}
	}

	if !isClientInRoom {
		return
	}

	if player.Id != room.taggedPlayerId {
		room.setTaggedPlayerUnsafe(player.Id)
	}
}

func (room *Room) SetTaggedPlayer(taggedPlayerId uint32) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if !room.canTag {
		return
	}

	var isClientInRoom bool
	for _, client := range room.Clients {
		if client.Id == taggedPlayerId {
			isClientInRoom = true
			break
		}
	}

	if !isClientInRoom {
		return
	}

	room.setTaggedPlayerUnsafe(taggedPlayerId)
}

func (room *Room) setTaggedPlayerUnsafe(taggedPlayerId uint32) {
	room.canTag = false
	room.taggedPlayerId = taggedPlayerId
	if room.tagCoolDown == 0 {
		room.tagCoolDown = 10 * time.Second
	}

	if room.cancelTagStart != nil {
		room.cancelTagStart()
		room.cancelTagStart = nil
	}

	if room.cancelTagCheck != nil {
		room.cancelTagCheck()
		room.cancelTagCheck = nil
	}

	room.sendMessageUnsafe(map[string]interface{}{
		"type":           "tagged",
		"taggedPlayerId": taggedPlayerId,
		"coolDown":       int(room.tagCoolDown.Seconds()),
	})

	ctx, cancelFn := context.WithCancel(context.Background())
	room.cancelTagStart = cancelFn

	go canTagNotifier(ctx, room, room.tagCoolDown)
}

func canTagNotifier(ctx context.Context, room *Room, duration time.Duration) {
	countdown := time.NewTimer(duration)
	select {
	case <-ctx.Done():
		return
	case <-countdown.C:
		room.EnableCanTag()
	}
}

func (room *Room) SendMessage(msg interface{}) {
	room.rwMu.RLock()
	defer room.rwMu.RUnlock()

	room.sendMessageUnsafe(msg)
}

func (room *Room) sendMessageUnsafe(msg interface{}) {
	for _, c := range room.Clients {
		go c.SendMessage(msg)
	}
}

func (room *Room) SendMessageExcept(id uint32, msg interface{}) {
	room.rwMu.RLock()
	defer room.rwMu.RUnlock()

	room.sendMessageExceptUnsafe(id, msg)
}

func (room *Room) sendMessageExceptUnsafe(id uint32, msg interface{}) {
	for _, c := range room.Clients {
		if c.Id != id {
			go c.SendMessage(msg)
		}
	}
}

func (room *Room) SendLastPackets(client *Client, conn net.PacketConn, addr net.Addr) {
	room.rwMu.RLock()
	defer room.rwMu.RUnlock()

	for _, c := range room.Clients {
		if c.Id != client.Id && c.lastPacket != nil && c.level == client.level {
			conn.WriteTo(c.lastPacket, addr)
		}
	}
}

func (room *Room) newPlayerTaggedLoop(ctx context.Context) {
	// roughly 63 times a second
	ticker := time.NewTicker(16 * time.Millisecond)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			if room.newPlayerTagged() {
				return
			}
		}
	}
}

func (room *Room) newPlayerTagged() bool {
	room.rwMu.RLock()
	taggedPlayerId := room.taggedPlayerId
	room.rwMu.RUnlock()

	currentTaggedClient := system.GetClientById(taggedPlayerId)
	if currentTaggedClient == nil {
		return false
	}

	taggedLevel, taggedPosition := currentTaggedClient.getLevelAndPosition()
	for _, other := range room.Clients {
		if other.Id == taggedPlayerId {
			continue
		}

		cLevel, cPosition := other.getLevelAndPosition()
		if cLevel == taggedLevel && distance(cPosition, taggedPosition) < 900 {
			room.SetTaggedPlayer(other.Id)
			return true
		}
	}

	return false
}
