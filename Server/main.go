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
	TagGameMode = "tag"
)

type Packet struct {
	Id uint32
}

type Client struct {
	Tcp        net.Conn
	Id         uint32
	rwMu       sync.RWMutex
	room       *Room
	name       string
	character  uint32
	level      string
	lastPacket []byte
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
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

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
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

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
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

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
	if room.cancelTag != nil {
		room.cancelTag()
		room.cancelTag = nil
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
		if c.Id != client.Id && c.lastPacket != nil && c.level == client.level {
			conn.WriteTo(c.lastPacket, addr)
		}
	}
}

type System struct {
	sync.RWMutex
	Rooms map[string]*Room
	rng   *rng
}

func (system *System) RemoveRoom(roomName string) {
	system.Lock()
	defer system.Unlock()

	delete(system.Rooms, roomName)

	log.Printf("deleted room %q", roomName)
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

		system.RLock()
		for _, r := range system.Rooms {
			r.SendMessage(map[string]interface{}{
				"type": "ping",
			})
		}
		system.RUnlock()
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
			client.room.StartTagGameMode()
		case "endGameMode":
			client.room.EndGameMode()
		case "tagged":
			taggedPlayerId, ok := getUint32Field(msg, "taggedPlayerId")
			if !ok {
				continue
			}

			client.room.SetTaggedPlayer(client, taggedPlayerId)
		case "disconnect":
			client.disconnectMsg()
		}
	}
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

		client := &Client{
			Tcp:  c,
			Id:   uuid.New().ID(),
			room: &Room{},
		}

		go client.tcpHandler()
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

			client.lastPacket = buf[:PacketSize]

			// Respond with the last packet of every other client in the same room and level
			client.room.SendLastPackets(client, server, addr)
		}()
	}
}
