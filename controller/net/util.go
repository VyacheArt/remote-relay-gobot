package net

import (
	"gobot.io/x/gobot"
	"gobot.io/x/gobot/platforms/firmata"
	"log"
	"reflect"
	"time"
	"unsafe"
)

func HealthChecker(device interface{ Toggle() error }, adaptor *firmata.TCPAdaptor, interval time.Duration) func() {
	return func() {
		gobot.Every(interval, func() {
			err := device.Toggle()
			if err != nil {
				log.Print("Seems like device is disconnected")
				log.Print(err)

				adaptor.Disconnect()

				sp, connErr := adaptor.PortOpener(adaptor.Port())
				if connErr != nil {
					log.Print("Failed to reconnect to device directly, will panic")
					panic(connErr)
				}

				connField := reflect.ValueOf(adaptor).Elem().FieldByName("conn")
				reflect.NewAt(connField.Type(), unsafe.Pointer(connField.UnsafeAddr())).
					Elem().
					Set(reflect.ValueOf(sp))

				err = adaptor.Connect()
				if err != nil {
					log.Print("Could not reconnect")
					log.Print(err)
					return
				}

				log.Print("Successfully reconnected")
			}
		})
	}
}
