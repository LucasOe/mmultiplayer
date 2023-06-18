package main

import (
	"encoding/binary"
	"encoding/json"
	"log"
	"net"
	"strings"
	"sync"
	"time"
)

const (
	CharacterFaith = iota
	CharacterKate
	CharacterCeleste
	CharacterAssaultCeleste
	CharacterJacknife
	CharacterMiller
	CharacterKreeg
	CharacterPersuitCop
	CharacterGhost
	CharacterMax
)

type Client struct {
	Tcp        net.Conn
	Id         uint32
	rwMu       sync.RWMutex
	room       *Room
	name       string
	character  uint32
	level      string
	lastPacket []byte
	position   position
}

func (client *Client) connectMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	msgRoom, ok := getTrimStringField(msg, "room")
	if !ok {
		return
	}

	msgName, ok := getTrimStringField(msg, "name")
	if !ok {
		return
	}

	msgLevel, ok := getTrimStringField(msg, "level")
	if !ok {
		return
	}

	msgCharacter, ok := msg["character"].(float64)
	if !ok || msgCharacter < 0 || msgCharacter >= CharacterMax {
		return
	}

	// Add the client to the room (create a new room if necessary)
	// TODO should we differentiate joining a room and creating a new one
	system.Lock()
	room, ok := system.Rooms[msgRoom]
	if !ok {
		room = &Room{
			Name: msgRoom,
		}

		system.Rooms[msgRoom] = room
		log.Printf("created room \"%s\"\n", room.Name)
	}
	system.Unlock()

	client.room = room
	client.name = msgName
	client.character = uint32(msgCharacter)
	client.level = strings.ToLower(msgLevel)

	// Tell the client their UUID
	// TODO consider calling on a go routine
	client.SendMessage(map[string]interface{}{
		"type": "id",
		"id":   client.Id,
	})

	room.AddPlayer(client)

	log.Printf("room \"%s\": \"%s\" joined\n", room.Name, client.name)
}

func (client *Client) nameMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	name, ok := getTrimStringField(msg, "name")
	if !ok {
		return
	}

	// Update the client's name and tell the other clients
	client.name = name
	client.room.SendMessageExcept(client.Id, map[string]interface{}{
		"type": "name",
		"id":   client.Id,
		"name": client.name,
	})
}

func (client *Client) chatMsg(msg map[string]interface{}) {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	body, ok := msg["body"].(string)
	if !ok {
		return
	}

	client.room.SendMessage(map[string]interface{}{
		"type": "chat",
		"body": client.name + ": " + body,
	})
}

func (client *Client) announceMsg(msg map[string]interface{}) {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	body, ok := msg["body"].(string)
	if !ok {
		return
	}

	client.room.SendMessage(map[string]interface{}{
		"type": "announce",
		"body": body,
	})
}

func (client *Client) cooldownMsg(msg map[string]interface{}) {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	cooldown, ok := getTimeDurationSecondsField(msg, "cooldown")
	if !ok {
		return
	}

	client.room.SetTagCooldown(cooldown)
}

func (client *Client) levelMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	level, ok := msg["level"].(string)
	if !ok {
		return
	}

	client.level = strings.ToLower(level)
	client.room.SendMessageExcept(client.Id, map[string]interface{}{
		"type":  "level",
		"id":    client.Id,
		"level": client.level,
	})
}

func (client *Client) characterMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	character, ok := msg["character"].(float64)
	if !ok || character < 0 || character >= CharacterMax {
		return
	}

	client.character = uint32(character)
	client.room.SendMessageExcept(client.Id, map[string]interface{}{
		"type":      "character",
		"id":        client.Id,
		"character": client.character,
	})
}

func (client *Client) startTagGameModeMsg() {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	client.room.StartTagGameMode()
}

func (client *Client) endGameModeMsg() {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	client.room.EndGameMode()
}

func (client *Client) deadMsg() {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	client.room.PlayerDied(client)
}

func (client *Client) disconnectMsg() {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	log.Printf("room \"%s\": \"%s\" left room\n", client.room.Name, client.name)

	client.room.OnPlayerDisconnect(client)
	client.room = &Room{}
}

func (client *Client) SendMessage(msg interface{}) {
	r, err := json.Marshal(msg)
	if err != nil {
		return
	}

	client.Tcp.Write(append(r, 0))
}

func (client *Client) getLevelAndPosition() (string, position) {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	return client.level, client.position
}

func (client *Client) setLastPacketAndPosition(buf []byte) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	client.lastPacket = buf

	x := float64(binary.LittleEndian.Uint32(buf[4:8]))
	y := float64(binary.LittleEndian.Uint32(buf[8:12]))
	z := float64(binary.LittleEndian.Uint32(buf[12:16]))
	client.position = position{x: x, y: y, z: z}
}

func getUint32Field(obj map[string]interface{}, field string) (uint32, bool) {
	vf, ok := obj[field].(float64)
	if !ok {
		return 0, false
	}

	return uint32(vf), true
}

func getTimeDurationSecondsField(obj map[string]interface{}, field string) (time.Duration, bool) {
	v, ok := obj[field].(float64)
	if !ok {
		return 0, false
	}

	return time.Duration(v * float64(time.Second)), true
}

func getTrimStringField(obj map[string]interface{}, field string) (string, bool) {
	v, ok := obj[field].(string)
	if !ok {
		return "", false
	}

	v = strings.TrimSpace(v)
	return v, v != ""
}

func (client *Client) tcpHandler() {
	defer client.Tcp.Close()

	d := json.NewDecoder(client.Tcp)

	for {
		client.Tcp.SetReadDeadline(time.Now().Add(30 * time.Second))

		var msg map[string]interface{}
		err := d.Decode(&msg)
		if err != nil {
			client.room.OnPlayerDisconnect(client)

			if client.name != "" {
				log.Printf("\"%s\" disconnected - %s", client.name, err)
			}

			return
		}

		client.Tcp.SetReadDeadline(time.Time{})

		msgType, ok := getTrimStringField(msg, "type")
		if !ok {
			continue
		}

		switch msgType {
		case "connect":
			client.connectMsg(msg)
		case "name":
			client.nameMsg(msg)
		case "chat":
			client.chatMsg(msg)
		case "announce":
			client.announceMsg(msg)
		case "cooldown":
			client.cooldownMsg(msg)
		case "level":
			client.levelMsg(msg)
		case "character":
			client.characterMsg(msg)
		case "startTagGameMode":
			client.startTagGameModeMsg()
		case "endGameMode":
			client.endGameModeMsg()
		case "dead":
			client.deadMsg()
		case "disconnect":
			client.disconnectMsg()
		}
	}
}
