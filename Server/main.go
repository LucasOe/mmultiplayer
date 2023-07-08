package main

import (
	"context"
	"crypto/rand"
	"encoding/binary"
	"log"
	"math"
	mathrand "math/rand"
	"net"
	"os/signal"
	"sync"
	"time"

	"github.com/Toyro98/mmultiplayer/Server/internal/osspecific"
	"github.com/google/uuid"
)

const (
	Port        = "5222"
	TagGameMode = "tag"
)

var system = System{Rooms: map[string]*Room{}}

func main() {
	ctx, cancelFn := signal.NotifyContext(context.Background(), osspecific.QuitSignals()...)
	defer cancelFn()

	randomNumberGenerator, err := newRng()
	if err != nil {
		log.Fatalf("failed to create random number - %s", err)
	}

	system.rng = randomNumberGenerator

	go tcpListener()
	go udpListener()

	go func() {
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
	}()

	<-ctx.Done()
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
			client := system.GetClientById(binary.LittleEndian.Uint32(buf[0:4]))
			if client == nil {
				return
			}

			// TODO combine this method and client.room.SendLastPackets into one method
			client.setLastPacketAndPosition(buf[:packetSize])

			// Respond with the last packet of every other client in the same room and level
			client.room.SendLastPackets(client, server, addr)
		}()
	}
}

type position struct {
	x float64
	y float64
	z float64
}

func distanceMeters(from position, to position) float64 {
	return math.Sqrt(math.Pow(to.x-from.x, 2)+math.Pow(to.y-from.y, 2)+math.Pow(to.z-from.z, 2)) / 100
}
