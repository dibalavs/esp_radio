<template>
  <div>
    <navbar v-model="curr_page"/>
    <div class="container" id="liveAlertPlaceholder">
      <keep-alive>
        <sysinfo v-if="curr_page==='home'" src_url="/sysinfo"/>
      </keep-alive>
      <keep-alive>
        <devices v-if="curr_page==='devices'" src_url="/devices/list"/>
      </keep-alive>
      <device v-if="curr_page==='device'" src_url="/devices/info"/>
      <settings v-if="curr_page==='settings'" src_url="/settings"/>
    </div>
     <alerts :items="alert_items"/>
  </div>
</template>

<script>
import navbar from './components/navbar.vue'
import sysinfo from './components/sysinfo.vue'
import devices from './components/devices.vue'
import device from './components/device.vue'
import settings from './components/settings.vue'
import alerts from './components/alerts.vue'

export default {
  name: 'ESP ZigBee',
  components: {
    navbar,
    sysinfo,
    device,
    devices,
    settings,
    alerts,
  },
  data() {
    return {
      curr_page: "home",
      curr_device: "",
      alert_items: [],
    }
  },
  watch: {
     curr_page: function(new_) {
        console.log("curr_page:" + new_)
      }
    }
}
</script>