//go:build !windows

package osspecific

import (
	"os"
	"syscall"
)

func QuitSignals() []os.Signal {
	return []os.Signal{syscall.SIGQUIT, syscall.SIGTERM, syscall.SIGINT}
}
