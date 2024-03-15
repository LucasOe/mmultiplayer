package main

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"io"
	"log"
	"math"
	"net"
	"net/http"
	"strings"
	"sync"
	"time"
)

var url = ""

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

type LeaderboardRunData []struct {
	PlayerName     string  `json:"PlayerName"`
	Distance       float64 `json:"Distance"`
	AvgSpeed       float64 `json:"AvgSpeed"`
	Time           float64 `json:"Time"`
	SkillRating    int     `json:"SkillRating"`
	IsDownloadable bool    `json:"IsDownloadable"`
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
		room = newRoom(msgRoom)

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
		"type":           "id",
		"id":             client.Id,
		"gameMode":       room.gameMode,
		"taggedPlayerId": room.taggedPlayerId,
		"canTag":         room.canTag,
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

func (client *Client) GetLevelAndPosition() (string, position) {
	client.rwMu.RLock()
	defer client.rwMu.RUnlock()

	return client.level, client.position
}

func (client *Client) setLastPacketAndPosition(buf []byte) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	client.lastPacket = buf

	x := float64(math.Float32frombits(binary.LittleEndian.Uint32(buf[4:8])))
	y := float64(math.Float32frombits(binary.LittleEndian.Uint32(buf[8:12])))
	z := float64(math.Float32frombits(binary.LittleEndian.Uint32(buf[12:16])))
	client.position = position{x: x, y: y, z: z}
}

func (client *Client) sendPostDataMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	jsonString, ok := msg["body"].(string)
	if !ok {
		log.Printf("Error post data is not a string")
		return
	}

	post, err := http.DefaultClient.Post(url+"/submit.php", "application/json", bytes.NewBuffer([]byte(jsonString)))
	if err != nil {
		log.Printf("Error sending post data")
		return
	}

	defer post.Body.Close()
}

func (client *Client) sendGetDataMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	gamemode, errorMsg := msg["gamemode"].(string)
	if !errorMsg {
		log.Printf("Error: gamemode is nil")
		return
	}

	course, errorMsg := msg["course"].(string)
	if !errorMsg {
		log.Printf("Error: course is nil")
		return
	}

	sortby, errorMsg := msg["sortby"].(string)
	if !errorMsg {
		log.Printf("Error: sortby is nil")
		return
	}

	timeframe, errorMsg := msg["timeframe"].(string)
	if !errorMsg {
		log.Printf("Error: timeframe is nil")
		return
	}

	params := "?gamemode=" + gamemode + "&course=" + course + "&sortby=" + sortby + "&timeframe=" + timeframe

	if sortby != "0" {
		player, errorMsg := msg["player"].(string)
		if !errorMsg {
			log.Printf("Error: player is nil")
			return
		}

		params = params + "&player=" + player
	}

	log.Printf(url + "/get.php" + params)

	get, err := http.DefaultClient.Get(url + "/get.php" + params)
	if err != nil {
		log.Printf("Error sending get request")
		return
	}

	defer get.Body.Close()

	body, err := io.ReadAll(get.Body)
	if err != nil {
		log.Printf("Error reading get data")
		return
	}

	/*
		// https://www.sohamkamani.com/golang/json/#json-arrays
		var leaderboard []LeaderboardRunData
		leaderboardError := json.Unmarshal(body, &leaderboard)

		if leaderboardError != nil {
			log.Printf("Error converting json into leaderboard struct")
			return
		}
	*/

	client.room.SendMessageTo(client.Id, map[string]interface{}{
		"type":       "leaderboard",
		"subtype":    "runs",
		"id":         client.Id,
		"data":       string(body),
		"statusCode": get.StatusCode,
	})
}

func (client *Client) sendLoginMsg(msg map[string]interface{}) {
	client.rwMu.Lock()
	defer client.rwMu.Unlock()

	username, errorMsg := msg["username"].(string)
	if !errorMsg {
		log.Printf("Error: username is nil")
		return
	}

	password, errorMsg := msg["password"].(string)
	if !errorMsg {
		log.Printf("Error: password is nil")
		return
	}

	get, err := http.DefaultClient.Get(url + "/signin.php?username=" + username + "&password=" + password)
	if err != nil {
		log.Printf("Error sending get request")
		return
	}

	defer get.Body.Close()

	body, err := io.ReadAll(get.Body)
	if err != nil {
		log.Printf("Error reading get data")
		return
	}

	client.room.SendMessageTo(client.Id, map[string]interface{}{
		"type":       "leaderboard",
		"subtype":    "login",
		"id":         client.Id,
		"data":       string(body),
		"statusCode": get.StatusCode,
	})
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
		case "leaderboard":
			msgSubType, ok := getTrimStringField(msg, "subtype")
			if !ok {
				continue
			}
			switch msgSubType {
			case "login":
				client.sendLoginMsg(msg)
			case "post":
				client.sendPostDataMsg(msg)
			case "get":
				client.sendGetDataMsg(msg)
			}
		}
	}
}
