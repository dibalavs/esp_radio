<template>
  <p>hello world: {{ this.$root.curr_device }}</p>
</template>

<script>
export default {
  name: "device",
  data() {
    return {
      items: "",
      timer: "",
    };
  },
};
</script>