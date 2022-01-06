package main

import (
	"controller/net"
	"controller/relayui"
	"flag"
	"fmt"
	"gobot.io/x/gobot"
	"gobot.io/x/gobot/drivers/gpio"
	"gobot.io/x/gobot/platforms/firmata"
	"log"
	"time"
)

const (
	relayPort = 3030
)

var (
	uiHost = flag.String("ui-host", "0.0.0.0", "UI host. 0.0.0.0 to listen on all interfaces")
	uiPort = flag.Int("ui-port", 3180, "UI port")
)

func main() {
	flag.Parse()

	log.Println("Discovering of relay. Reset your device")
	deviceHost, deviceType, err := net.Discover(30 * time.Second)
	if err != nil {
		panic(err)
	}

	log.Printf("Device found! IP: %s, type: %s", deviceHost, deviceType.String())

	firmataAdaptor := firmata.NewTCPAdaptor(fmt.Sprintf("%s:%d", deviceHost, relayPort))

	relay := gpio.NewRelayDriver(firmataAdaptor, "5")
	relay.Inverted = true

	builtInLed := gpio.NewLedDriver(firmataAdaptor, "2")

	robot := gobot.NewRobot("remote-relay",
		[]gobot.Connection{firmataAdaptor},
		[]gobot.Device{relay, builtInLed},
		net.HealthChecker(builtInLed, firmataAdaptor, 5*time.Second),
	)

	go relayui.StartServer(*uiHost, uint16(*uiPort), relay)

	robot.Start()
}
