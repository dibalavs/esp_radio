import { rest } from 'msw'

const api_prefix = "/api/v1/"
var is_fm = false
var is_playing = false
var station_no = 6
var volume = 42

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

var system_info = rest.get("sysinfo", (req, res, ctx) => {
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
        name: 'Zigbee',
        values: [{ name: "IEEE addr", value: "0x123456789" },
        { name: "Revision", value: "1.2.3.4" },
        { name: "PAN id ", value: "0x1234" },
        { name: "Channel", value: "13" },
        { name: "Status", value: "0x09" }]
      },
      {
        name: 'MQTT',
        values: [{ name: "Status", value: "not connected" },
        { name: "Prefix", value: "none" }]
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

export default [player_info, player_pause, player_volume, player_prev, player_next, player_fmradio, player_ipradio, system_info,
  rest.get('/settings', (req, res, ctx) => {
    return res(
     // ctx.status(403),
      ctx.json({
        zigbee_rx: 22,
        zigbee_tx: 23,
        zigbee_power: 20,
        zigbee_channel: 11,
        zigbee_panid: "12ab",
        wifi_hostname: "zb_host",
        wifi_mode: "AP",
        wifi_ap_ssid: "ssid_ap",
        wifi_ap_password: "ssid_password",
        wifi_ssid: "ssid",
        wifi_password: "password",
        ntp_pool: "europe.pool.ntp.org",
        mqtt_server: "test.mqtt.org"
      }))
  }),
  rest.post('/settings', (req, res, ctx) => {
    console.log("reqbody:" + req.body)
    return res(
      // ctx.status(403),
      ctx.json({
        status: 0, //0  - success; !0 - failure
        errors: ["settings error1", "settings error2", "settings error3"]
      }))
  }),
  rest.get('/devices/action/start_join', (req, res, ctx) => {
    var timeout = req.url.searchParams.get('timeout_sec')
    console.log("Start joining for " + timeout + " seconds.")
    return res(
      ctx.status(200)
    )
  }),
  rest.get('/devices/action/stop_join', (req, res, ctx) => {
    console.log("Stop joining.")
    return res(
      ctx.status(200)
    )
  })
]

// rest.get('/settings', (req, res, ctx) => {
//  return res(
//    ctx.json({
//    }))
// })