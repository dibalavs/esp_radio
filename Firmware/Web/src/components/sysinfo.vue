<template>
  <div class="row row-cols-1 row-cols-md-2 row-cols-lg-3 g-1 justify-content-md-center">
    <template v-for="item in items" :key="item.name">
      <div class="col" style="margin-bottom: 1rem">
        <div class="card text-center h-100" style="min-width: 18rem">
          <div class="card-header">
            {{ item.name }}
          </div>
          <div class="card-body">
            <template v-for="value in item.values" :key="value.name">
              <p class="card-text" style="margin-bottom: 0rem">
                <b>{{ value.name }}:</b> {{ value.value }}
              </p>
            </template>
          </div>
        </div>
      </div>
    </template>
  </div>
</template>

<script>
export default {
  name: "sysinfo",
  props: ["src_url"],
  activated() {
    this.getItems();
    this.timer = setInterval(this.getItems, 3000);
  },
  deactivated() {
    this.clearTimer()
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
      fetch(this.api_prefix + "sysinfo")
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
          this.alert("Sysinfo failed", error)
          console.error("Request failed", error);
        });
    },
  },
};
</script>