<template>
  <nav class="navbar navbar-expand-md navbar-dark bg-primary mb-4">
    <div class="container-fluid">
      <a class="navbar-brand" href="#">ESP Radio</a>
      <button
        class="navbar-toggler"
        type="button"
        data-bs-toggle="collapse"
        data-bs-target="#navbarCollapse"
        aria-controls="navbarCollapse"
        aria-expanded="false"
        aria-label="Toggle navigation"
      >
        <span class="navbar-toggler-icon"></span>
      </button>
      <div class="collapse navbar-collapse" id="navbarCollapse">
        <ul class="navbar-nav me-auto mb-2 mb-md-0">
          <template v-for="item in menu_items" :key="item.id">
            <li class="nav-item">
              <a
                class="nav-link"
                :class="{ active: modelValue === item.id }"
                @click="this.$emit('update:modelValue', item.id)"
                aria-current="page"
                href="#"
                >{{ item.title }}</a
              >
            </li>
          </template>
        </ul>
      </div>
    </div>
  </nav>
</template>

<script>
export default {
  name: "navbar",
  props: ["modelValue"],
  emits: ["update:modelValue"],
  data() {
    return {
      menu_items: [
        { id: "home", title: "Home" },
        { id: "ipradio_list", title: "IP radio" },
        { id: "fmradio_list", title: "FM radio" },
        { id: "settings", title: "Settings" },
        { id: "help", title: "Help" },
      ],
    };
  },
};
</script>