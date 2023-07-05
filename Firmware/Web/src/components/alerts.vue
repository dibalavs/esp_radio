<template>
<div class="position-fixed bottom-0 end-0 p-3" style="z-index: 11">
  <template v-for="item in items" :key="item.id">
    <div class="alert alert-danger alert-dismissible fade show" role="alert">
      <p><b>{{item.title}}</b>
        <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
      </p>
      {{item.message}}

    </div>
  </template>

</div>
</template>

<script>
export default {
  name: "alerts",
  props: ["items"],
  data() {
    return {item:1};
  },
};
</script>

<style scoped>
  .alert {
    padding:0.5rem
  }
  .alert-dismissible {
    padding-right:2rem
  }
  .btn-close {
    padding:0.5rem
  }
  p {
    margin-bottom:0
  }
</style>
