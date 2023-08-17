<template>
  <div>
    <navbar v-model="curr_page"/>
    <div class="container" id="liveAlertPlaceholder">
      <keep-alive>
        <sysinfo v-if="curr_page==='home'" src_url="/sysinfo"/>
      </keep-alive>
      <keep-alive>
        <ipradio_list v-if="curr_page==='ipradio_list'"/>
      </keep-alive>
      <keep-alive>
        <fmradio_list v-if="curr_page==='fmradio_list'"/>
      </keep-alive>
      <settings v-if="curr_page==='settings'" src_url="/settings"/>
    </div>
    <alerts :items="alert_items"/>
  </div>
  <footer class="footer">
      <player/>
  </footer>
</template>
<style>
.footer {
  position: absolute;
  bottom: 0;
  width: 96%;
  height: 60px; /* Set the fixed height of the footer here */
  line-height: 50px; /* Vertically center the text there */
}
</style>
<script>
import navbar from './components/navbar.vue'
import sysinfo from './components/sysinfo.vue'
import settings from './components/settings.vue'
import alerts from './components/alerts.vue'
import player from './components/player.vue'
import ipradio_list from './components/ipradio_list.vue'
import fmradio_list from './components/fmradio_list.vue'

export default {
  name: 'ESP ZigBee',
  components: {
    navbar,
    sysinfo,
    settings,
    alerts,
    player,
    ipradio_list,
    fmradio_list
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