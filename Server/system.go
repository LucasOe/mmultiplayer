package main

import (
	"log"
	"sync"
)

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
