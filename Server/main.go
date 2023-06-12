package main

import (
	"crypto/rand"
	"encoding/binary"
	"log"
	mathrand "math/rand"
	"net"
	"sync"
	"time"
	"unsafe"

	"github.com/google/uuid"
)

const (
	Port        = "5222"
	TagGameMode = "tag"
)

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
	const packetSize = 676

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

		if n != packetSize {
			continue
		}

		go func() {
			client := system.GetClientById((*Packet)(unsafe.Pointer(&buf[0])).Id)
			if client == nil {
				return
			}

			client.lastPacket = buf[:packetSize]

			// Respond with the last packet of every other client in the same room and level
			client.room.SendLastPackets(client, server, addr)
		}()
	}
}

type Packet struct {
	Id uint32
}
