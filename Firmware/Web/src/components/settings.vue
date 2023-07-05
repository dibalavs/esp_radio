<template>
  <div>
    <!-- Modal window with reboot notification -->
    <div class="modal fade" id="rebootRequest" tabindex="-1" aria-hidden="false">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title"> Warning!</h5>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
          </div>
          <div class="modal-body">
            Settings were successfully stored. To apply settings you need reboot your device. Please reboot.
          </div>
          <div class="modal-footer">
            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Close</button>
          </div>
        </div>
      </div>
    </div>

  <form @submit.prevent="doSubmit()">
    <div class="row row-cols-2 row-cols-md-2 row-cols-lg-2 g-1 justify-content-md-center" v-if="item">
      <div class="col h-100">
        <div class="card" style="min-width: 18rem">
          <div class="card-header">ZigBee</div>
          <div class="card-body">
            <div class="input-group mb-3">
              <span class="input-group-text" id="zigbee-txpin" style="width: 10rem">ZigBee TX pin</span>
              <input
                type="number"
                v-model="this.item.zigbee_tx"
                class="form-control"
                placeholder="TX pin number"
                aria-label="TX pin number"
                aria-describedby="zigbee-txpin"
              />
            </div>

            <div class="input-group mb-3">
              <span class="input-group-text" id="zigbee-rxpin" style="width: 10rem">ZigBee RX pin</span>
              <input
                type="number"
                v-model="this.item.zigbee_rx"
                class="form-control"
                placeholder="RX pin number"
                aria-label="RX pin number"
                aria-describedby="zigbee-rxpin"
              />
            </div>

            <div class="input-group mb-3">
              <span class="input-group-text" id="zigbee-power" style="width: 7rem">Power:</span>
              <span class="input-group-text" id="zigbee-power-lbl" style="width: 3rem" name="zigbee-power-lbl">{{
                item.zigbee_power
              }}</span>
              <input
                type="range"
                v-model="this.item.zigbee_power"
                style="margin-left: 1rem"
                class="form-control-range"
                min="1"
                max="20"
                step="1"
                aria-describedby="zigbee-power-lbl"
              />
            </div>

            <div class="input-group mb-3">
              <span class="input-group-text" id="zigbee-channel" style="width: 7rem">Channel:</span>
              <span class="input-group-text" id="zigbee-channel-lbl" style="width: 3rem" name="zigbee-channel-lbl">{{
                item.zigbee_channel
              }}</span>
              <input
                type="range"
                v-model="this.item.zigbee_channel"
                style="margin-left: 1rem"
                class="form-control-range"
                min="11"
                max="26"
                step="1"
                aria-describedby="zigbee-channel-lbl"
              />
            </div>

            <div class="input-group mb-3">
              <span class="input-group-text" id="zigbee-panid" style="width: 10rem">PAN id 0x</span>
              <input
                type="text"
                v-model="this.item.zigbee_panid"
                pattern="[A-Fa-f0-9]+"
                maxlength="4"
                class="form-control"
                placeholder="PAN id in hex format"
                aria-label="PAN id in hex format"
                aria-describedby="zigbee-panid"
              />
            </div>
          </div>
        </div>
      </div>

      <div class="col">
        <div class="card h-100" style="min-width: 18rem">
          <div class="card-header">WiFi</div>
          <div class="card-body">
            <div class="input-group mb-3">
              <span class="input-group-text" style="width: 9rem">Hostname</span>
              <input
                type="text"
                v-model="this.item.wifi_hostname"
                class="form-control"
                placeholder="DHCP hostname"
                aria-label="DHCP hostname"
                aria-describedby="wifi-hostname"
              />
            </div>

            <div class="input-group mb-3">
              <label class="input-group-text" style="width: 9rem">Mode</label>
              <select class="form-select" v-model="item.wifi_mode">
                <option value="AP">Access Point</option>
                <option value="STA">Station</option>
              </select>
            </div>

            <div class="input-group mb-3" v-show="item.wifi_mode === 'AP'">
              <span class="input-group-text" style="width: 9rem" id="wifi-ap-ssid">AP SSID</span>
              <input
                type="text"
                v-model="this.item.wifi_ap_ssid"
                class="form-control"
                placeholder="SSID for Access Point mode"
                aria-label="SSID for Access Point mode"
                aria-describedby="wifi-ap-ssid"
              />
            </div>

            <div class="input-group mb-3" v-show="item.wifi_mode === 'AP'">
              <span class="input-group-text" style="width: 9rem" id="wifi-ap-password">AP Password</span>
              <input
                type="text"
                v-model="this.item.wifi_ap_password"
                class="form-control"
                placeholder="Password for Access Point mode"
                aria-label="Password for Access Point mode"
                aria-describedby="wifi-ap-password"
              />
            </div>

            <div class="input-group mb-3" v-show="item.wifi_mode === 'STA'">
              <span class="input-group-text" style="width: 9rem" id="wifi-ssid">STA SSID</span>
              <input
                type="text"
                v-model="this.item.wifi_ssid"
                class="form-control"
                placeholder="SSID for Station mode"
                aria-label="SSID for Station mode"
                aria-describedby="wifi-ssid"
              />
            </div>

            <div class="input-group mb-3" v-show="item.wifi_mode === 'STA'">
              <span class="input-group-text" style="width: 9rem" id="wifi-password">STA Password</span>
              <input
                type="password"
                v-model="this.item.wifi_password"
                class="form-control"
                placeholder="Password for Station mode"
                aria-label="Password for Station mode"
                aria-describedby="wifi-password"
              />
            </div>
          </div>
        </div>
      </div>
      <button type="submit" class="btn btn-primary">Submit</button>
    </div>
  </form>
  </div>
</template>

<script>
export default {
  name: "settings",
  props: ["src_url"],
  created() {
    this.getItem();
  },
  data() {
    return {
      item: "",
    };
  },
  methods: {
    getItem() {
      fetch(this.src_url)
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          return response.json();
        })
        .then((json) => {
          this.item = json;
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Get settings failed", error);
        });
    },
    doSubmit() {
      fetch(this.src_url, {
        method: "POST",
        headers: new Headers({
          'Content-Type': 'application/json'
        }),
        body: JSON.stringify(this.item) })
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          return response.json();
        })
        .then((json) => {
          if (json.status == 0) {
            /*global bootstrap*/
            var myModal = new bootstrap.Modal(document.getElementById('rebootRequest'))
            myModal.show()
            //this.$root.curr_page = "home";
          } else {
            this.alert("Set settings error", "Status code:" + json.status);
            json.errors.forEach(element => {
              this.alert("Error", element)
            });
          }
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Set settings failed", error);
        });
    },
  },
};
</script>