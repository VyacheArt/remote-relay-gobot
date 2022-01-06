package net

import (
	"context"
	"net"
	"strings"
	"time"
)

const (
	BroadcastPort = 3090
)

const (
	Relay DeviceType = "IOTBRDCSTRELAY"
)

type DeviceType string

func (t DeviceType) String() string {
	switch t {
	case Relay:
		return "relay"

	default:
		return "unknown"
	}
}

func Discover(timeout time.Duration) (host string, deviceType DeviceType, err error) {
	conn, err := net.ListenUDP("udp", &net.UDPAddr{
		Port: BroadcastPort,
		IP:   net.ParseIP("0.0.0.0"),
	})

	if err != nil {
		return "", "", err
	}

	defer conn.Close()

	type result struct {
		Host  string
		Type  DeviceType
		Error error
	}

	resultCh := make(chan result, 1)

	go func() {
		for {
			message := make([]byte, 20)
			rlen, remote, err := conn.ReadFromUDP(message[:])
			if err != nil {
				resultCh <- result{Error: err}
				return
			}

			data := strings.TrimSpace(string(message[:rlen]))
			switch data {
			case string(Relay):
				resultCh <- result{Host: remote.IP.String(), Type: Relay}
				return
			}
		}
	}()

	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	select {
	case res := <-resultCh:
		return res.Host, res.Type, res.Error

	case <-ctx.Done():
		return "", "", context.DeadlineExceeded
	}
}
