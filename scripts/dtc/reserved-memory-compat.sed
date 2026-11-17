# Preserve both SDM845 reserved-memory labels for legacy perseus overlays.
s/^\([[:space:]]*\)reserved_mem:[[:space:]]*reserved-memory[[:space:]]*{/\1reserved_memory: reserved_mem: reserved-memory {/
s/^\([[:space:]]*\)reserved_memory:[[:space:]]*reserved-memory[[:space:]]*{/\1reserved_memory: reserved_mem: reserved-memory {/
