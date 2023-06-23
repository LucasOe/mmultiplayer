package main

import (
	"context"
	"fmt"
	"net"
	"sync"
	"time"
)

func newRoom(name string) *Room {
	return &Room{
		Name:    name,
		Clients: make(map[uint32]*Client),
	}
}

type Room struct {
	Name           string
	rwMu           sync.RWMutex
	Clients        map[uint32]*Client
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

	room.Clients[client.Id] = client

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
	for randPlayerId, player := range room.Clients {
		room.setTaggedPlayerUnsafe(randPlayerId, player.name+" will chase")
		break
	}
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

	delete(room.Clients, disconnectedPlayer.Id)

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

	if room.gameMode != TagGameMode || player.Id == room.taggedPlayerId {
		return
	}

	room.setTaggedPlayerUnsafe(player.Id, player.name+" died and they will chase instead")
}

func (room *Room) setTaggedPlayerUnsafe(taggedPlayerId uint32, msg string) {
	_, isClientInRoom := room.Clients[taggedPlayerId]
	if !isClientInRoom {
		return
	}

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

	room.announceUnsafe("[Tag] " + msg)

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

func (room *Room) announceUnsafe(msg string) {
	room.sendMessageUnsafe(map[string]interface{}{
		"type": "announce",
		"body": msg,
	})
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
	defer room.rwMu.RUnlock()

	if !room.canTag {
		return false
	}

	currentTaggedClient := system.GetClientById(room.taggedPlayerId)
	if currentTaggedClient == nil {
		return false
	}

	taggedLevel, taggedPosition := currentTaggedClient.getLevelAndPosition()
	for _, other := range room.Clients {
		if other.Id == room.taggedPlayerId {
			continue
		}

		cLevel, cPosition := other.getLevelAndPosition()
		if cLevel == taggedLevel && distance(cPosition, taggedPosition) < 900 {
			room.setTaggedPlayerUnsafe(other.Id, fmt.Sprintf("%s was tagged by %s",
				currentTaggedClient.name, other.name))
			return true
		}
	}

	return false
}
