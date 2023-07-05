import { rest } from 'msw'

export default [
  rest.get('/sysinfo', (req, res, ctx) => {
    return res(
//      ctx.status(403),
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
  }),
  rest.get('/devices/list', (req, res, ctx) => {
    return res(
      //ctx.status(403),
      ctx.json([
        {
          name: 'friendly name1',
          code: "product code",
          model: "product model1",
          manuf: "manuf name 1",
          addr: "short addr",
          ieee: "ieee addr1",
          buildid: "build id1",
          power: "power1"
        },

        {
          name: 'friendly name2',
          code: "product code2",
          model: "product model2",
          manuf: "manuf name 2",
          addr: "short addr",
          ieee: "ieee addr2",
          buildid: "build id2",
          power: "power2"
        },

        {
          name: 'friendly name3',
          code: "product code3",
          model: "product model3",
          manuf: "manuf name 3",
          addr: "short addr3",
          ieee: "ieee addr3",
          buildid: "build id3",
          power: "power3"
        },

        {
          name: 'friendly name4',
          code: "product code4",
          model: "product model4",
          manuf: "manuf name 4",
          addr: "short addr4",
          ieee: "ieee addr4",
          buildid: "build id4",
          power: "power4"
        },
      ]))
  }),
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