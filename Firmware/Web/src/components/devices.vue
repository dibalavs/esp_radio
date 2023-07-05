<template>
  <div>
    <DevicesToolbar/>
    <table class="table table-striped table-hover">
      <thead>
        <tr>
          <th scope="col">#</th>
          <th scope="col">Friendly name</th>
          <th scope="col">Id</th>
          <th scope="col">Model</th>
          <th scope="col">Manuf</th>
          <th scope="col">Addr</th>
          <th scope="col">IEEE addr</th>
          <th scope="col">BuildId</th>
          <th scope="col">Power</th>
          <!-- <th scope="col">Actions</th> -->
        </tr>
      </thead>
      <tbody>
        <template v-for="(item, index) in items" :key="item.addr">
          <tr role="button" data-href="#" @click="onRowClicked(item.ieee)">
            <th scope="row">{{ index }}</th>
            <td>{{ item.name }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.model }}</td>
            <td>{{ item.manuf }}</td>
            <td>{{ item.addr }}</td>
            <td>{{ item.ieee }}</td>
            <td>{{ item.buildid }}</td>
            <td>{{ item.power }}</td>
            <!--  <td></td>  -->
          </tr>
        </template>
      </tbody>
    </table>
  </div>
</template>

<script>
import DevicesToolbar from './devices_toolbar.vue'

export default {
  name: "devices",
  components: {
    DevicesToolbar
  },
  props: ["src_url"],
  activated() {
    this.getItems();
    this.timer = setInterval(this.getItems, 5000);
  },
  deactivated() {
    this.clearTimer();
  },
  data() {
    return {
      items: "",
      timer: "",
    };
  },
  methods: {
    clearTimer() {
      if (this.timer)
        clearInterval(this.timer);
      this.timer = null
    },
    getItems() {
      fetch(this.src_url)
        .then((response) => {
          if (!response.ok) {
            throw new Error("HTTP status " + response.status);
          }
          return response.json();
        })
        .then((json) => {
          this.items = json;
        })
        .catch((error) => {
          this.clearTimer()
          console.error("Request failed", error);
          this.alert("Get devices failed", error);
        });
    },
    onRowClicked(ieee) {
      this.$root.curr_device = ieee;
      console.log("iddd:" + ieee);
      this.$root.curr_page = "device";
    },
  },
};
</script>