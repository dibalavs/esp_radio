<template>
  <div class="btn-toolbar" role="toolbar">
    <div class="btn-group me-2" role="group">
      <button v-if="!isJoining" type="button" class="btn btn-primary" @click="onStartJoin()">Join</button>
      <button v-if="isJoining" type="button" class="btn btn-primary" @click="onStopJoin()">Stop Join</button>
    </div>
    <div v-if="isJoining" class="input-group">
      <div class="input-group-text">Joining devices (sec):</div>
      <input type="text" disabled="true" size="1" class="form-control" v-model="joinTimeLeft">
    </div>
  </div>
</template>

<script>
export default {
  name: "DevicesToolbar",
  activated() {
  },
  deactivated() {
    this.onStopJoin();
  },
  data() {
    return {
      isJoining: false,
      joinTimer: null,
      joinTimerTick: null,
      joinTimeLeft: 0,
      joinTimeoutSec: 120
    };
  },
  methods: {
    onStopJoin() {
      if (this.joinTimer) {
        clearInterval(this.joinTimer);
        fetch("/devices/action/stop_join")
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Stop joining failed", error);
        });
      }

      if (this.joinTimerTick)
        clearInterval(this.joinTimerTick);

      this.joinTimer = null
      this.joinTimerTick = null
      this.isJoining = false
    },
    onStartJoin() {
      this.joinTimeLeft = this.joinTimeoutSec
      this.joinTimer = setInterval(this.onStopJoin, this.joinTimeLeft * 1000)
      this.joinTimerTick = setInterval(() => { this.joinTimeLeft-- }, 1000)
      this.isJoining = true

      fetch('/devices/action/start_join?' + new URLSearchParams({
          timeout_sec:this.joinTimeoutSec
      }))
        .then((response) => {
          console.log(response.statusText)
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Start joining failed", error);
          this.onStopJoin()
        });
    },
  },
};
</script>