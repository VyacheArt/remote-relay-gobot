package relayui

import (
	_ "embed"
	"fmt"
	"gobot.io/x/gobot/drivers/gpio"
	"log"
	"net/http"
)

var (
	//go:embed button_off.html
	buttonOffTemplate string

	//go:embed button_on.html
	buttonOnTemplate string
)

type Server struct {
	relay *gpio.RelayDriver
}

func StartServer(ip string, port uint16, relay *gpio.RelayDriver) {
	s := &Server{
		relay: relay,
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/", s.index)

	log.Printf("Start HTTP server on %s:%d\n", ip, port)
	http.ListenAndServe(fmt.Sprintf("%s:%d", ip, port), mux)
}

func (s *Server) index(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodPost {
		r.ParseForm()

		shouldBeEnabled := r.Form.Get("enabled") == "1"
		if shouldBeEnabled {
			s.relay.On()
		} else {
			s.relay.Off()
		}
	}

	if s.relay.State() {
		w.Write([]byte(buttonOffTemplate))
	} else {
		w.Write([]byte(buttonOnTemplate))
	}
}
