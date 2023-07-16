// Application that helps evade Windows Defender by adding a Windows
// Defender exclusion and XORing launcher and dll prior to extracting
// those files at install-time.
package main

import (
	"context"
	_ "embed"
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
)

var (
	//go:embed launcher.buh
	launcherXOR []byte

	//go:embed dll.buh
	dllXOR []byte
)

func main() {
	var logF *os.File

	// Create a log file if a debug env. is not specified.
	if os.Getenv("SQUIBBLES_DEBUG_NO_LOG") != "true" {
		// Note: '*' is replaced with a random number.
		logF, _ = os.CreateTemp("", "mirrors-edge-multiplayer-installer-helper-*.log")
		if logF != nil {
			log.SetOutput(logF)
		}
	}

	exitCode := 0

	err := mainWithError()
	if err != nil {
		log.Println("fatal:", err)
		exitCode = 1
	}

	if logF != nil {
		_ = logF.Sync()
		_ = logF.Close()
	}

	os.Exit(exitCode)
}

func mainWithError() error {
	flag.Parse()

	ctx, cancelFn := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancelFn()

	switch flag.Arg(0) {
	case "install":
		return install(ctx)
	case "uninstall":
		return uninstall(ctx)
	default:
		return fmt.Errorf("unknown non-flag argument: '%s'", flag.Arg(0))
	}
}

func install(ctx context.Context) error {
	exePath, err := os.Executable()
	if err != nil {
		return fmt.Errorf("failed to get exe path - %w", err)
	}

	parentDirectory := filepath.Dir(exePath)

	// Windows Defender does not like how the launcher
	// does process injection (it may also dislike the
	// client dll for unknown reasons). The long-term
	// soltuion for this is to modify the launcher's
	// process injection behavior to not make Defender
	// angy. For now, we are opting to add a Defender
	// exclusion for the "bin" directory.
	//
	// While sub-optimal, a Defender exclsuion should
	// be safe because the installer writes the files
	// to a directory that only Windows Administrators
	// can write to. This means malware and other
	// unsavory programs cannot abuse our exclusion
	// to hide from Defender.
	//
	// We do this in squibbles instead of in the installer
	// because Windows 11's Defender appears to test for
	// this behavior (it is unclear how it actually
	// detects it). This does not appear to be the
	// case on Windows 10.
	//
	// Note: The extra double quotes around the parent
	// directory are required. Without these quotes,
	// the string will be malformed if a single quote
	// is present in the directory path. This is some
	// terrible Windows garbage where each process
	// handles argv differently (rather than it being
	// an array that is passed to the process like
	// a sane operating system would do).
	output, err := exec.CommandContext(ctx,
		`C:\WINDOWS\system32\WindowsPowerShell\v1.0\powershell.exe`,
		"Add-MpPreference",
		"-ExclusionPath",
		`"`+parentDirectory+`"`).CombinedOutput()
	if err != nil {
		return fmt.Errorf("failed to add windows defender exclusion for '%s' - output: '%s' - %w",
			parentDirectory, output, err)
	}

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

func uninstall(ctx context.Context) error {
	exePath, err := os.Executable()
	if err != nil {
		return fmt.Errorf("failed to get exe path - %w", err)
	}

	parentDirectory := filepath.Dir(exePath)

	// Note: The extra double quotes around the parent
	// directory are required. Without these quotes,
	// the string will be malformed if a single quote
	// is present in the directory path.
	output, err := exec.CommandContext(ctx,
		`C:\WINDOWS\system32\WindowsPowerShell\v1.0\powershell.exe`,
		"Remove-MpPreference",
		"-ExclusionPath",
		`"`+parentDirectory+`"`).CombinedOutput()
	if err != nil {
		return fmt.Errorf("failed to remove windows defender exclusion for '%s' - output: '%s' - %w",
			parentDirectory, output, err)
	}

	return nil
}
