package main

import (
	"context"
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
	// TODO remove room variables once Toyro knows about the change
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
	for randPlayerId := range room.Clients {
		room.setTaggedPlayerUnsafe(randPlayerId)
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
}

func (room *Room) PlayerDied(player *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if room.gameMode != TagGameMode || player.Id == room.taggedPlayerId {
		return
	}

	room.setTaggedPlayerUnsafe(player.Id)
}

func (room *Room) setTaggedPlayerUnsafe(taggedPlayerId uint32) {
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

	// TODO mutex hat :(
	for _, c := range room.Clients {
		if c.Id != client.Id && c.lastPacket != nil && c.level == client.level {
			conn.WriteTo(c.lastPacket, addr)
		}
	}

	if !room.canTag {
		return
	}

	taggedClient := room.Clients[room.taggedPlayerId]
	if taggedClient == nil || client == taggedClient {
		return
	}

	clientLevel, clientPosition := client.GetLevelAndPosition()
	taggedLevel, taggedPosition := taggedClient.GetLevelAndPosition()

	if clientLevel == taggedLevel && distanceMeters(clientPosition, taggedPosition) < 1.3 {
		go func() {
			room.rwMu.Lock()
			defer room.rwMu.Unlock()

			room.setTaggedPlayerUnsafe(client.Id)
		}()
		return
	}
}
