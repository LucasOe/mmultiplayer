package main

import (
	"context"
	"crypto/rand"
	"encoding/binary"
	"encoding/json"
	"log"
	mathrand "math/rand"
	"net"
	"strings"
	"sync"
	"time"
	"unsafe"

	"github.com/google/uuid"
)

const (
	Port           = "5222"
	PacketSize     = 676
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

type Packet struct {
	Id uint32
}

type Client struct {
	Tcp        net.Conn
	Id         uint32
	Room       *Room
	Name       string
	Character  uint32
	Level      string
	LastPacket []byte
	LastSeen   time.Time
}

func (client *Client) SendMessage(msg interface{}) {
	r, err := json.Marshal(msg)
	if err != nil {
		return
	}

	client.Tcp.Write(append(r, 0))
}

type Room struct {
	rwMu           sync.RWMutex
	Name           string
	Clients        []*Client
	gameMode       string
	taggedPlayerId uint32
	canTag         bool
	cancelTag      func()
	tagCoolDown    time.Duration
}

func (room *Room) AddPlayer(client *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.Clients = append(room.Clients, client)

	// Notify the other clients that a new client connected
	room.sendMessageExceptUnsafe(client.Id, map[string]interface{}{
		"type":      "connect",
		"id":        client.Id,
		"name":      client.Name,
		"character": client.Character,
		"level":     client.Level,
	})

	// Tell the client the existing clients
	for _, c := range room.Clients {
		if c.Id != client.Id {
			go client.SendMessage(map[string]interface{}{
				"type":           "connect",
				"id":             c.Id,
				"name":           c.Name,
				"character":      c.Character,
				"level":          c.Level,
				"taggedPlayerId": room.taggedPlayerId,
				"canTag":         room.canTag,
			})
		}
	}
}

func (room *Room) StartTagGameMode() {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	room.gameMode = "tag"

	room.sendMessageUnsafe(map[string]interface{}{
		"type":     "gameMode",
		"gameMode": "tag",
	})

	room.tagRandomPlayerUnsafe()
}

func (room *Room) TagRandomPlayer() {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

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
	if room.cancelTag != nil {
		room.cancelTag()
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

func (room *Room) DisconnectIdlePlayers() int {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if len(room.Clients) == 0 {
		return 0
	}

	var newClients []*Client
	for _, c := range room.Clients {
		if time.Since(c.LastSeen) > 30*time.Second {
			room.removePlayerUnsafe(c)
			log.Printf("timed out %x \"%s\"\n", c.Id, c.Name)
		} else {
			newClients = append(newClients, c)
		}
	}

	room.Clients = newClients
	return len(newClients)
}

func (room *Room) OnPlayerDisconnect(disconnectedPlayer *Client) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	var newClients []*Client

	for _, c := range room.Clients {
		if c.Id != disconnectedPlayer.Id {
			newClients = append(newClients, c)
		}
	}

	room.Clients = newClients
	room.removePlayerUnsafe(disconnectedPlayer)
}

func (room *Room) removePlayerUnsafe(player *Client) {
	go room.SendMessageExcept(player.Id, map[string]interface{}{
		"type": "disconnect",
		"id":   player.Id,
	})

	if player.Id == room.taggedPlayerId {
		if room.cancelTag != nil {
			room.cancelTag()
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
}

func (room *Room) SetTaggedPlayer(currentTaggedPlayer *Client, taggedPlayerId uint32) {
	room.rwMu.Lock()
	defer room.rwMu.Unlock()

	if currentTaggedPlayer.Id != room.taggedPlayerId {
		return
	}

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

	room.sendMessageUnsafe(map[string]interface{}{
		"type":           "tagged",
		"taggedPlayerId": taggedPlayerId,
		"coolDown":       int(room.tagCoolDown.Seconds()),
	})

	ctx, cancelFn := context.WithCancel(context.Background())
	room.cancelTag = cancelFn

	go canTagNotifier(ctx, room, room.tagCoolDown)
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
		if c.Id != client.Id && c.LastPacket != nil && c.Level == client.Level {
			conn.WriteTo(c.LastPacket, addr)
		}
	}
}

type System struct {
	sync.RWMutex
	Rooms map[string]*Room
	rng   *rng
}

func (system *System) GetClientById(id uint32) *Client {
	system.RLock()
	defer system.RUnlock()

	for _, r := range system.Rooms {
		for _, c := range r.Clients {
			if c.Id == id {
				return c
			}
		}
	}

	return nil
}

var system = System{Rooms: map[string]*Room{}}

func main() {
	randomNumberGenerator, err := newRng()
	if err != nil {
		log.Fatalf("failed to create random number - %s", err)
	}

	system.rng = randomNumberGenerator

	go tcpListener()
	go udpListener()

	for {
		time.Sleep(2 * time.Second)

		system.Lock()
		for name, r := range system.Rooms {
			r.SendMessage(map[string]interface{}{
				"type": "ping",
			})

			if r.DisconnectIdlePlayers() == 0 {
				delete(system.Rooms, name)
			}
		}
		system.Unlock()
	}
}

func newRng() (*rng, error) {
	buh := make([]byte, 8)
	_, err := rand.Read(buh)
	if err != nil {
		return nil, err
	}

	return &rng{
		r: mathrand.New(mathrand.NewSource(int64(binary.LittleEndian.Uint64(buh)))),
	}, nil
}

type rng struct {
	mu sync.Mutex
	r  *mathrand.Rand
}

func (o *rng) Intn(n int) int {
	o.mu.Lock()
	result := o.r.Intn(n)
	o.mu.Unlock()
	return result
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

func tcpHandler(c net.Conn) {
	d := json.NewDecoder(c)

	for {
		var msg map[string]interface{}
		err := d.Decode(&msg)
		if err != nil {
			break
		}

		msgType, ok := getTrimStringField(msg, "type")
		if !ok {
			continue
		}

		switch msgType {
		case "connect":
			msgRoom, ok := getTrimStringField(msg, "room")
			if !ok {
				continue
			}

			msgName, ok := getTrimStringField(msg, "name")
			if !ok {
				continue
			}

			msgLevel, ok := getTrimStringField(msg, "level")
			if !ok {
				continue
			}

			msgCharacter, ok := msg["character"].(float64)
			if !ok || msgCharacter < 0 || msgCharacter >= CharacterMax {
				continue
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

			client := &Client{
				Tcp:        c,
				Id:         uuid.New().ID(),
				Room:       room,
				Name:       msgName,
				Character:  uint32(msgCharacter),
				Level:      strings.ToLower(msgLevel),
				LastPacket: nil,
				LastSeen:   time.Now(),
			}

			// Tell the client their UUID
			client.SendMessage(map[string]interface{}{
				"type": "id",
				"id":   client.Id,
			})

			room.AddPlayer(client)

			log.Printf("room \"%s\": \"%s\" joined\n", room.Name, client.Name)
		case "name":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			name, ok := getTrimStringField(msg, "name")
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			// Update the client's name and tell the other clients
			client.Name = name
			client.LastSeen = time.Now()
			client.Room.SendMessageExcept(client.Id, map[string]interface{}{
				"type": "name",
				"id":   client.Id,
				"name": client.Name,
			})
		case "chat":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			body, ok := msg["body"].(string)
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.LastSeen = time.Now()
			client.Room.SendMessage(map[string]interface{}{
				"type": "chat",
				"body": client.Name + ": " + body,
			})
		case "announce":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			body, ok := msg["body"].(string)
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.LastSeen = time.Now()
			client.Room.SendMessage(map[string]interface{}{
				"type": "announce",
				"body": body,
			})
		case "cooldown":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			cooldown, ok := getTimeDurationSecondsField(msg, "cooldown")
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.Room.SetTagCooldown(cooldown)

			client.LastSeen = time.Now()
		case "level":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			level, ok := msg["level"].(string)
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.Level = strings.ToLower(level)
			client.LastSeen = time.Now()
			client.Room.SendMessageExcept(client.Id, map[string]interface{}{
				"type":  "level",
				"id":    client.Id,
				"level": client.Level,
			})
		case "character":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			character, ok := msg["character"].(float64)
			if !ok || character < 0 || character >= CharacterMax {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.Character = uint32(character)
			client.LastSeen = time.Now()
			client.Room.SendMessageExcept(client.Id, map[string]interface{}{
				"type":      "character",
				"id":        client.Id,
				"character": client.Character,
			})
		case "pong":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.LastSeen = time.Now()
		case "startTagGameMode":
			id, ok := getUint32Field(msg, "id")
			if !ok {
				continue
			}

			client := system.GetClientById(id)
			if client == nil {
				continue
			}

			client.Room.StartTagGameMode()
		case "endGameMode":
			id, ok := getUint32Field(msg, "id")
			if !ok {
				continue
			}

			client := system.GetClientById(id)
			if client == nil {
				continue
			}

			client.Room.EndGameMode()
		case "tagged":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			taggedPlayerId, ok := getUint32Field(msg, "taggedPlayerId")
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}
			client.LastSeen = time.Now()

			client.Room.SetTaggedPlayer(client, taggedPlayerId)
		case "disconnect":
			id, ok := msg["id"].(float64)
			if !ok {
				continue
			}

			client := system.GetClientById(uint32(id))
			if client == nil {
				continue
			}

			client.Room.OnPlayerDisconnect(client)

			log.Printf("room \"%s\": \"%s\" disconnected\n", client.Room.Name, client.Name)
		}
	}

	c.Close()
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

func tcpListener() {
	server, err := net.Listen("tcp4", ":"+Port)
	if err != nil {
		log.Fatalln(err)
	}

	log.Println("tcp listener started")

	for {
		c, err := server.Accept()
		if err != nil {
			continue
		}

		go tcpHandler(c)
	}
}

func udpListener() {
	server, err := net.ListenPacket("udp", ":"+Port)
	if err != nil {
		log.Fatalln(err)
	}

	log.Println("udp listener started")

	for {
		buf := make([]byte, 0xFFF)
		n, addr, err := server.ReadFrom(buf)
		if err != nil {
			continue
		}

		if n != PacketSize {
			continue
		}

		go func() {
			client := system.GetClientById((*Packet)(unsafe.Pointer(&buf[0])).Id)
			if client == nil {
				return
			}

			client.LastPacket = buf[:PacketSize]
			client.LastSeen = time.Now()

			// Respond with the last packet of every other client in the same room and level
			client.Room.SendLastPackets(client, server, addr)
		}()
	}
}
