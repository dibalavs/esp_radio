import { rest } from 'msw'

const api_prefix = "/api/v1/"
var is_fm = false
var is_playing = false
var station_no = 6
var volume = 42

var all_ip_stations = [
  {no:0, name:"станция1", url:"http://blabla1.ru:234/list", ovol:0},
  {no:1, name:"станция2", url:"http://blabla2.ru:234/list", ovol:1},
  {no:2, name:"станция3", url:"http://blabla3.ru:234/list", ovol:2},
  {no:3, name:"станция4", url:"http://blabla4.ru:234/list", ovol:3},
  {no:4, name:"станция5", url:"http://blabla5.ru:234/list", ovol:4}
]

var all_fm_stations = [
  {no:0, name:"радио1", frequency:"88.1", ovol:0},
  {no:1, name:"радио2", frequency:"89.9", ovol:1},
  {no:2, name:"радио3", frequency:"101.1", ovol:2},
  {no:3, name:"станция4", frequency:"102.2", ovol:3},
  {no:4, name:"станция5", frequency:"103.3", ovol:4}
]

////////////////////////////////////////////////////////////////////////////
// Manage player

var player_info = rest.get(api_prefix + "player_info", (req, res, ctx) => {
  return res(
    //ctx.status(403),
    ctx.json(
      {
        is_fm: is_fm,
        is_playing: is_playing,
        volume: volume,
        station_type: is_fm ? "FM": "Ip",
        station_no: station_no,
        station_name: is_fm ? "Детское радио" : "Радио на 7 холмах",
        track_name: is_playing ? "Музыка для сна." : ""
      }))
})

var player_pause = rest.get(api_prefix + "player_pause", (req, res, ctx) => {
  if (is_playing) {
    console.log("Stop playing.")
    is_playing = false
  } else {
    console.log("Start playing")
    is_playing = true
  }

  return res(
    ctx.status(200)
  )
})

var player_volume = rest.post(api_prefix + "player_volume", async (req, res, ctx) => {
  const { value } = await req.json()
  console.log("Set new volume:" + value)
  volume = value
  return res(
    ctx.status(200)
  )
})

var player_prev = rest.get(api_prefix + "player_prev", (req, res, ctx) => {
  station_no--
  console.log("Switch station:" + station_no)
  return res(
    ctx.status(200)
  )
})

var player_next = rest.get(api_prefix + "player_next", (req, res, ctx) => {
  station_no++
  console.log("Switch station:" + station_no)
  return res(
    ctx.status(200)
  )
})

var player_fmradio = rest.get(api_prefix + "player_fmradio", (req, res, ctx) => {
  console.log("Switch to FM radio" )
  is_fm = true;
  return res(
    ctx.status(200)
  )
})

var player_ipradio = rest.get(api_prefix + "player_ipradio", (req, res, ctx) => {
  console.log("Switch to IP radio" )
  is_fm = false;
  return res(
    ctx.status(200)
  )
})

//////////////////////////////////////////////////////////////////////////////////////
// ipradio_list

var ipradio_list = rest.get(api_prefix + "ipradio_list", (req, res, ctx) => {
  return res(
    //ctx.status(403),
    ctx.json(all_ip_stations))
})

var ipradio_import = rest.post(api_prefix + "ipradio_import", async (req, res, ctx) => {
  const js = await req.json()
  all_ip_stations = js
  return res(
    //ctx.status(403),
    ctx.status(200))
})

var ipradio_set = rest.post(api_prefix + "ipradio_set", async (req, res, ctx) => {
  const { station_no:no } = await req.json()
  console.log("Set new station:" + no)
  station_no = no
  is_fm = false
  return res(
    ctx.status(200)
  )
})

var ipradio_move = rest.post(api_prefix + "ipradio_move", async (req, res, ctx) => {
  const { curr, prev } = await req.json()
  var first = all_ip_stations.find(item => item.no == curr)
  var second = all_ip_stations.find(item => item.no == prev)
  first.no = prev
  second.no = curr
  console.log("Swap stations:" + curr + "," + prev)
  return res(
    ctx.status(200)
  )
})

var ipradio_export = rest.get(api_prefix + "ipradio_export", (req, res, ctx) => {
  return res(
    //ctx.status(403),
    ctx.json(all_ip_stations))
})

//////////////////////////////////////////////////////////////////////////////////////
// fmradio_list

var fmradio_list = rest.get(api_prefix + "fmradio_list", (req, res, ctx) => {
  return res(
    //ctx.status(403),
    ctx.json(all_fm_stations))
})

var fmradio_set = rest.post(api_prefix + "fmradio_set", async (req, res, ctx) => {
  const { radio_no:no } = await req.json()
  console.log("Set new radio:" + no)
  station_no = no
  is_fm = true
  return res(
    ctx.status(200)
  )
})

var fmradio_move = rest.post(api_prefix + "fmradio_move", async (req, res, ctx) => {
  const { curr, prev } = await req.json()
  var first = all_fm_stations.find(item => item.no == curr)
  var second = all_fm_stations.find(item => item.no == prev)
  first.no = prev
  second.no = curr
  console.log("Swap radios:" + curr + "," + prev)
  return res(
    ctx.status(200)
  )
})

var fmradio_import = rest.post(api_prefix + "fmradio_import", async (req, res, ctx) => {
  const js = await req.json()
  all_fm_stations = js
  return res(
    //ctx.status(403),
    ctx.status(200))
})

var fmradio_export = rest.get(api_prefix + "fmradio_export", (req, res, ctx) => {
  return res(
    //ctx.status(403),
    ctx.json(all_fm_stations))
})

///////////////////////////////////////////////////////////////////////////////////////////
var system_info = rest.get(api_prefix + "sysinfo", (req, res, ctx) => {
  return res(
    ctx.json([
      {
        name: 'System',
        values: [{ name: "version", value: "1234" },
        { name: "Uptime", value: "12:22" },
        { name: "Heap", value: "12345" },
        { name: "PSRAM total", value: "4445567" },
        { name: "PSRAM free", value: "12345" }]
      },
      {
        name: 'WiFi',
        values: [{ name: "Mode", value: "Access Point" },
        { name: "SSID", value: "asdfasf" },
        { name: "IP", value: "1.2.3.4" },
        { name: "MAC address", value: "12:24:45:45:56:67" },
        { name: "Singal", value: "-45 Dbm" },
        { name: "Hostname", value: "esp_zigbee" }]
      },
      {
        name: 'Time',
        values: [{ name: "Uptime", value: "12:12" },
        { name: "NTP server", value: "pool.ntp.ru" },
        { name: "Current time", value: "12:12:12" }]
      },
    ])
  )
})

export default [player_info, player_pause, player_volume, player_prev, player_next, player_fmradio, player_ipradio,
                ipradio_list, ipradio_set, ipradio_move, ipradio_import, ipradio_export,
                fmradio_list, fmradio_set, fmradio_move, fmradio_import, fmradio_export,
  system_info,
  rest.get(api_prefix + 'settings', (req, res, ctx) => {
    return res(
     // ctx.status(403),
      ctx.json({
        audio_mode: "VS1053",
        treble: "2",
        treble_freq: "10",
        bass: "5",
        bass_freq: "10",

        wifi_hostname: "esp_host",
        wifi_mode: "AP",
        wifi_ap_ssid: "ssid_ap",
        wifi_ap_password: "ssid_password",
        wifi_ssid: "ssid",
        wifi_password: "password",
        ntp_pool: "europe.pool.ntp.org"

      }))
  }),
  rest.post(api_prefix + 'settings', (req, res, ctx) => {
    console.log("reqbody:" + req.body)
    return res(
      // ctx.status(403),
      ctx.json({
        status: 0, //0  - success; !0 - failure
        errors: ["settings error1", "settings error2", "settings error3"]
      }))
  }),
]

// rest.get('/settings', (req, res, ctx) => {
//  return res(
//    ctx.json({
//    }))
// })