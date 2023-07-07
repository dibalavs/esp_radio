<template>
  <div class="bg container">
    <div class="row row-cols-auto g-0">
      <div class="col">
        <button type="button" @click="onPrevChannel()" class="btn" data-toggle="tooltip" title="Previous channel" >
          <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" fill="currentColor" class="bi bi-rewind" viewBox="0 0 16 16">
            <path d="M9.196 8 15 4.633v6.734L9.196 8Zm-.792-.696a.802.802 0 0 0 0 1.392l6.363 3.692c.52.302 1.233-.043 1.233-.696V4.308c0-.653-.713-.998-1.233-.696L8.404 7.304Z"/>
            <path d="M1.196 8 7 4.633v6.734L1.196 8Zm-.792-.696a.802.802 0 0 0 0 1.392l6.363 3.692c.52.302 1.233-.043 1.233-.696V4.308c0-.653-.713-.998-1.233-.696L.404 7.304Z"/>
          </svg>
        </button>

        
        <button type="button" @click="onFmRadio()" class="nopad btn" v-if="isFmRadio" data-toggle="tooltip" title="FM Radio" >
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-r-circle" viewBox="0 0 16 16">
            <path d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8Zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0ZM5.5 4.002h3.11c1.71 0 2.741.973 2.741 2.46 0 1.138-.667 1.94-1.495 2.24L11.5 12H9.98L8.52 8.924H6.836V12H5.5V4.002Zm1.335 1.09v2.777h1.549c.995 0 1.573-.463 1.573-1.36 0-.913-.596-1.417-1.537-1.417H6.835Z"/>
          </svg>
        </button>
        <button type="button" @click="onIpRadio()" class="nopad btn" v-if="!isFmRadio" data-toggle="tooltip" title="Web Radio" >
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-circle" viewBox="0 0 16 16">
            <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z"/>
          </svg>
        </button>


        <button type="button" @click="onPause()" class="nopad btn" v-if="!isPlaying" data-toggle="tooltip" title="Playing stopped" >
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-stop-circle" viewBox="0 0 16 16">
            <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z"/>
            <path d="M5 6.5A1.5 1.5 0 0 1 6.5 5h3A1.5 1.5 0 0 1 11 6.5v3A1.5 1.5 0 0 1 9.5 11h-3A1.5 1.5 0 0 1 5 9.5v-3z"/>
          </svg>
        </button>
        <button type="button" @click="onPause()" v-if="isPlaying" class="nopad btn" data-toggle="tooltip" title="Playing">
          <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-play-circle" viewBox="0 0 16 16">
            <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z"/>
            <path d="M6.271 5.055a.5.5 0 0 1 .52.038l3.5 2.5a.5.5 0 0 1 0 .814l-3.5 2.5A.5.5 0 0 1 6 10.5v-5a.5.5 0 0 1 .271-.445z"/>
          </svg>
        </button>


        <button type="button" @click="onNextChannel()" class="btn" data-toggle="tooltip" title="Next channel" >
          <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" fill="currentColor" class="bi bi-fast-forward" viewBox="0 0 16 16">
            <path d="M6.804 8 1 4.633v6.734L6.804 8Zm.792-.696a.802.802 0 0 1 0 1.392l-6.363 3.692C.713 12.69 0 12.345 0 11.692V4.308c0-.653.713-.998 1.233-.696l6.363 3.692Z"/>
            <path d="M14.804 8 9 4.633v6.734L14.804 8Zm.792-.696a.802.802 0 0 1 0 1.392l-6.363 3.692C8.713 12.69 8 12.345 8 11.692V4.308c0-.653.713-.998 1.233-.696l6.363 3.692Z"/>
          </svg>
        </button>
      </div>
      
      <div class="col">
        <svg xmlns="http://www.w3.org/2000/svg" v-if="volume == 0" width="24" height="24" fill="currentColor" class="bi bi-volume-mute" style="margin-left: 1rem" viewBox="0 0 16 16">
          <path d="M6.717 3.55A.5.5 0 0 1 7 4v8a.5.5 0 0 1-.812.39L3.825 10.5H1.5A.5.5 0 0 1 1 10V6a.5.5 0 0 1 .5-.5h2.325l2.363-1.89a.5.5 0 0 1 .529-.06zM6 5.04 4.312 6.39A.5.5 0 0 1 4 6.5H2v3h2a.5.5 0 0 1 .312.11L6 10.96V5.04zm7.854.606a.5.5 0 0 1 0 .708L12.207 8l1.647 1.646a.5.5 0 0 1-.708.708L11.5 8.707l-1.646 1.647a.5.5 0 0 1-.708-.708L10.793 8 9.146 6.354a.5.5 0 1 1 .708-.708L11.5 7.293l1.646-1.647a.5.5 0 0 1 .708 0z"/>
        </svg>
        <svg xmlns="http://www.w3.org/2000/svg" v-if="volume > 0" width="24" height="24" fill="currentColor" class="bi bi-volume-off" style="margin-left: 1rem" viewBox="0 0 16 16">
          <path d="M10.717 3.55A.5.5 0 0 1 11 4v8a.5.5 0 0 1-.812.39L7.825 10.5H5.5A.5.5 0 0 1 5 10V6a.5.5 0 0 1 .5-.5h2.325l2.363-1.89a.5.5 0 0 1 .529-.06zM10 5.04 8.312 6.39A.5.5 0 0 1 8 6.5H6v3h2a.5.5 0 0 1 .312.11L10 10.96V5.04z"/>
        </svg>

        <input type="range" v-model="this.volume" class="form-control-range align-middle" min="0" max="200" step="1" style="accent-color: black;" data-toggle="tooltip" title="Volume" />
      </div>

      <div class="col gx-3">
        <div class="display-6">{{station_type}}:</div>
      </div>

      <div class="col">
        <div>{{station_no}}:&nbsp;&nbsp;{{station_name}}&nbsp;-&nbsp;{{track_name}}</div>
      </div>
    </div>
  </div>
</template>
<style>
.hline { width:100%; height:1px; background: #2a2a2a70 }
.bg { background: blanchedalmond; }
.nopad { padding: 0em !important}
</style>
<script>
export default {
  name: "player",
  props: ["src_url"],
  created() {
    this.getInfo();
    this.timer = setInterval(this.getInfo, 2000);
  },
  destroyed() {
    this.clearTimer()
  },
  data() {
    return {
      timer: "",
      isPlaying: false,
      isFmRadio: false,
      station_type: "",
      station_no: 0,
      station_name: "",
      track_name: "",
      volume: 0
    };
  },
  methods: {
    clearTimer() {
      if (this.timer)
        clearInterval(this.timer);
      this.timer = null
    },
    getInfo() {
      fetch(this.api_prefix + "player_info")
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          return response.json();
        })
        .then((json) => {
          this.isFmRadio = json.is_fm
          this.isPlaying = json.is_playing
          this.volume = json.volume
          this.station_type = json.station_type
          this.station_no = json.station_no
          this.station_name = json.station_name
          this.track_name = json.track_name
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Get settings failed", error);
        });
    },
    onPrevChannel(){
      fetch(this.api_prefix + "player_prev").then((response) => {
        this.getInfo()
      })
    },
    onNextChannel(){
      fetch(this.api_prefix + "player_next").then((response) => {
        this.getInfo()
      })
    },
    onPause(){
      fetch(this.api_prefix + "player_pause").then((response) => {
        this.getInfo()
      })
    },
    onFmRadio(){
      fetch(this.api_prefix + "player_ipradio").then((response) => {
        this.getInfo()
      })
    },
    onIpRadio(){
      fetch(this.api_prefix + "player_fmradio").then((response) => {
        this.getInfo()
      })
    }
  },
  watch: {
    volume(new_value) {
      this.volume = new_value      
      fetch(this.api_prefix + "player_volume", {
        method: "POST",
        headers: new Headers({
          'Content-Type': 'application/json'
        }),
        body: JSON.stringify({value: new_value}) 
      })
    }
  }
};
</script>