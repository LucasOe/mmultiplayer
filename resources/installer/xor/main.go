package main

import (
	"errors"
	"flag"
	"fmt"
	"log"
	"os"
)

func main() {
	err := mainWithError()
	if err != nil {
		log.Fatalf("fatal: %s", err)
	}

}

func mainWithError() error {
	flag.Parse()

	if flag.NArg() < 1 {
		return errors.New("missing a file")
	}

	file, err := os.ReadFile(flag.Arg(0))
	if err != nil {
		return fmt.Errorf("cannot read file - %s", err)
	}

	for _, v := range file {
		_, err := os.Stdout.Write([]byte{v ^ 4})
		if err != nil {
			return fmt.Errorf("failed to write to standard out - %s", err)
		}
	}

	return nil
}
