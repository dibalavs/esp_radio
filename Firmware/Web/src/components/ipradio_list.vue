<template>
  <div>
    <IpRadioToolbar/>
    <table class="table table-striped table-hover table-sm">
      <thead>
        <tr>
          <th>#</th>
          <th class="w-50">Name</th>
          <th >URL</th>
          <th >Volume</th>
          <th >Edit</th>
        </tr>
      </thead>
      <tbody>
        <template v-for="(item, index) in sorted" :key="item.no">
          <tr role="button" data-href="#">
            <th scope="row" @click="onSwitchStation(item.no)">{{ item.no }}</th>
            <td @click="onSwitchStation(item.no)">{{ item.name }}</td>
            <td @click="onSwitchStation(item.no)">{{ item.url }}</td>
            <td @click="onSwitchStation(item.no)">{{ item.ovol }}</td>
            <td >
              <button type="button" class="btn" :style="item.no == 0 ? {'border-color': 'transparent'} : {}" :disabled="item.no == 0" @click="onSwapStations(item.no, item.no - 1)">
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-arrow-up-circle" viewBox="0 0 16 16">
                  <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z"/>
                </svg>
              </button>
              <button type="button" class="btn" :style="index == (items.length - 1) ? {'border-color': 'transparent'} : {}" :disabled="index == (items.length - 1)" @click="onSwapStations(item.no, item.no + 1)">
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-arrow-down-circle" viewBox="0 0 16 16">
                  <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z"/>
                </svg>
              </button>
              <button type="button" class="btn">Edit</button>
            </td>
          </tr>
        </template>
      </tbody>
    </table>
  </div>
</template>

<script>
import IpRadioToolbar from './ipradio_toolbar.vue'

export default {
  name: "ipradio_list",
  components: {
    IpRadioToolbar
  },
  props: [],
  activated() {
    this.getItems();
  },
  deactivated() {
  },
  data() {
    return {
      items: [],
    };
  },
  computed: {
    sorted() {
      return this.items.sort(function(a, b){ return a.no - b.no})
    }
  },
  methods: {
    getItems() {
      fetch(this.api_prefix + "ipradio_list")
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          return response.json();
        })
        .then((json) => {
          this.items = json
        })
        .catch((error) => {
          this.clearTimer()
          console.error("Request failed", error);
          this.alert("Get devices failed", error);
        });
    },
    onSwitchStation(station_no) {
      fetch(this.api_prefix + "ipradio_set", {
        method: "POST",
        headers: new Headers({
          'Content-Type': 'application/json'
        }),
        body: JSON.stringify({station_no: station_no})
      })
    },
    onSwapStations(curr, prev) {
      fetch(this.api_prefix + "ipradio_move", {
        method: "POST",
        headers: new Headers({
          'Content-Type': 'application/json'
        }),
        body: JSON.stringify({curr, prev})
      }).then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          this.getItems()
        })
        .catch((error) => {
          console.error("Request failed", error);
          this.alert("Move station failed", error);
        });
    }
  },
};
</script>