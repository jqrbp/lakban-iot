package main

var pIn = [5]int{5, 6, 13, 19, 26}
var lbls = [5]string{"All OFF", "TV Display", "TV Modem", "WIFI", "ISP Modem"}
var pOut = [5]int{0, 12, 21, 16, 20}
var toggleFlag bool
var pNum int
var ipTgtFoundFlag = false
var ipTgtFound = ""
var ipTgtPort = "8088"
var ipWebSocketPort = "81"
var wsProxyPort = "9193"
var httpProxyPort = "9192"
var httpPort = "9191"
