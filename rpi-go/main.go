package main

import (
    "log"

    "github.com/buaazp/fasthttprouter"
    "github.com/valyala/fasthttp"

    "github.com/kidoman/embd"
    _ "github.com/kidoman/embd/host/all"

    "./templates"
    "strconv"

    "net"
    "net/url"
    "net/http"
    "net/http/httputil"

    "io"
    "io/ioutil"
    "time"
)

func idHandler(ctx *fasthttp.RequestCtx) {
  ctx.SetBody([]byte("arpi-local"))
  ctx.SetContentType("text/plain; charset=utf-8")
}

func pinHandler(ctx *fasthttp.RequestCtx) {
	pnum := ctx.PostArgs().Peek("o")
//	pval := ctx.PostArgs().Peek("v")

	pNum, _ = strconv.Atoi(string(pnum))

	toggleFlag = true
	for (toggleFlag == true) {
		log.Println(".")
	}
	homeHandler(ctx)
}

func homeHandler(ctx *fasthttp.RequestCtx) {
    oAr := make([]int, len(pOut))
    copy(oAr[:],pOut[:])

    lblAr := make([]string, len(lbls))
    copy(lblAr[:],lbls[:])

    valAr := make([]int, len(pOut))

    for i:=0;i<len(pOut);i++ {
	if(pOut[i] > 0) {
		valo, _ := embd.DigitalRead(pOut[i])
		valAr[i] = valo
	}
    }

    p := &templates.Home{
	      OUT: oAr,
        LBL: lblAr,
        VAL: valAr,
        IPTGTADDR: ipTgtFound + ":" + ipTgtPort,
        WEBPROXYADDR: ctx.LocalIP().String() + ":" + httpProxyPort,
        WSPROXYADDR: ctx.LocalIP().String() + ":" + wsProxyPort + "/ws",
    }
    templates.WritePageHome(ctx, p)
    ctx.SetContentType("text/html; charset=utf-8")
}

func initPin() {
    embd.InitGPIO()
    defer embd.CloseGPIO()

    for i:=0; i < len(pIn); i++ {
  	embd.SetDirection(pIn[i], embd.In)
    }

    for i:=0; i < len(pOut); i++ {
	if pOut[i] > 0 {
		log.Println("set",pOut[i])
	        embd.SetDirection(pOut[i], embd.Out)
        	embd.DigitalWrite(pOut[i], embd.High)
	}
    }

    pinListener()
}

func togglePin(pinNum int) {
	if pinNum > 0 {
 	       valo, _ := embd.DigitalRead(pinNum)
               if valo > 0 {
                    embd.DigitalWrite(pinNum, embd.Low)
               } else {
                    embd.DigitalWrite(pinNum, embd.High)
               }
        } else {
		for i:=0; i<len(pOut);i++ {
			if pOut[i] > 0 {
				embd.DigitalWrite(pOut[i], embd.High)
			}
		}
	}
}

func pinListener() {
    pIn_old := make([]int, len(pIn))
    for {
	if (toggleFlag == true) {
		toggleFlag = false
		togglePin(pNum)
	}

	for i:=0; i < len(pIn); i++ {
		val, _ := embd.DigitalRead(pIn[i])
		if (pIn_old[i] != val) {
			if (pIn_old[i] > val) {
				togglePin(pOut[i])
			}
			pIn_old[i] = val
		}
	}
    }
}

func checkIP(_ipStr string) (bool, error) {
  log.Println("Checking: " + _ipStr)
  timeout := time.Duration(1 * time.Second)
  client := http.Client{
    Timeout: timeout,
  }

	resp, err := client.Get("http://" + _ipStr +":" + ipTgtPort + "/id")

	if err != nil {
	 return false, err
	}

  defer resp.Body.Close()
	body, _ := ioutil.ReadAll(resp.Body)

  if string(body) == "wemos-shift-web" {
		ipTgtFound = _ipStr
		return true, nil
  }
	return false, nil
}

func proxyHandler(p *httputil.ReverseProxy) func(http.ResponseWriter, *http.Request) {
    return func(w http.ResponseWriter, r *http.Request) {
        log.Println(r.URL)
        r.URL.Path = "/"
            p.ServeHTTP(w, r)
    }
}

func websocketProxy(target string) http.Handler {
        return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
                d, err := net.Dial("tcp", target)
                if err != nil {
                        http.Error(w, "Error contacting backend server.", 500)
                        log.Printf("Error dialing websocket backend %s: %v", target, err)
                        return
                }
                hj, ok := w.(http.Hijacker)
                if !ok {
                        http.Error(w, "Not a hijacker?", 500)
                        return
                }
                nc, _, err := hj.Hijack()
                if err != nil {
                        log.Printf("Hijack error: %v", err)
                        return
                }
                defer nc.Close()
                defer d.Close()

                err = r.Write(d)
                if err != nil {
                        log.Printf("Error copying request to target: %v", err)
                        return
                }

                errc := make(chan error, 2)
                cp := func(dst io.Writer, src io.Reader) {
                        _, err := io.Copy(dst, src)
                        errc <- err
                }
                go cp(d, nc)
                go cp(nc, d)
                <-errc
        })
}

func initProxyServer() {
  for(ipTgtFoundFlag == false) {
      for i := 100; i < 254; i++ {
          str:= "192.168.219." + strconv.Itoa(i);
          foundFlag, err := checkIP(str)
          if err != nil {
            continue
          }

          if(foundFlag == true) {
            ipTgtFoundFlag = true
            break
          }
      }
  }

  println("Found ip Address: " + ipTgtFound)

  // webServer Proxy
  go func() {
    remote, err := url.Parse("http://" + ipTgtFound + ":" + ipTgtPort)
    if err != nil {
            log.Println(err)
    }

    proxy := httputil.NewSingleHostReverseProxy(remote)
    http.HandleFunc("/", proxyHandler(proxy))
    err = http.ListenAndServe(":" + httpProxyPort, nil)
    if err != nil {
            log.Println(err)
    }
  }()

  // webSocket Proxy
  go func() {
    http.Handle("/ws", websocketProxy(ipTgtFound + ":" + ipWebSocketPort))
    err := http.ListenAndServe(":" + wsProxyPort, nil)
    if err != nil {
            log.Println(err)
    }
  }()
}

func main() {
    go func() {
      initProxyServer()
    }()

    go func() {
    	initPin()
    }()

    router := fasthttprouter.New()
    router.POST("/", pinHandler)
    router.GET("/", homeHandler)
    router.GET("/id", idHandler)

    log.Fatal(fasthttp.ListenAndServe(":" + httpPort, router.Handler))
}
