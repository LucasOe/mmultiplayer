// Application that helps evade Windows Defender by XORing launcher and dll and
// extracting files at install time.
package main

import (
	_ "embed"
	"fmt"
	"log"
	"os"
	"path/filepath"
)

var (
	//go:embed launcher.buh
	launcherXOR []byte

	//go:embed dll.buh
	dllXOR []byte
)

func main() {
	log.SetFlags(0)

	err := mainWithError()
	if err != nil {
		log.Fatalln("error:", err)
	}
}

func mainWithError() error {
	exePath, err := os.Executable()
	if err != nil {
		return fmt.Errorf("failed to get file path - %w", err)
	}

	parentDirectory := filepath.Dir(exePath)
	launcherF, err := os.OpenFile(filepath.Join(parentDirectory, "Launcher.exe"), os.O_CREATE|os.O_WRONLY, 0755)
	if err != nil {
		return fmt.Errorf("cannot open launcher - %w", err)
	}
	defer launcherF.Close()

	dllF, err := os.OpenFile(filepath.Join(parentDirectory, "mmultiplayer.dll"), os.O_CREATE|os.O_WRONLY, 0755)
	if err != nil {
		return fmt.Errorf("cannot open dll - %w", err)
	}
	defer dllF.Close()

	for _, v := range launcherXOR {
		_, err := launcherF.Write([]byte{v ^ 4})
		if err != nil {
			return fmt.Errorf("failed to write to launcher - %w", err)
		}
	}

	for _, v := range dllXOR {
		_, err := dllF.Write([]byte{v ^ 4})
		if err != nil {
			return fmt.Errorf("failed to write to dll - %w", err)
		}
	}

	return nil
}
