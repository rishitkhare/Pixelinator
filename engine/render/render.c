#include "render.h"
#include "../global.h"
#include "render_internal.h"
#include "../draw/bitmap.h"

static Render_State_Internal state = {0};

void render_init(u32 width, u32 height, Render_Flags flags) {
    SDL_Init(SDL_INIT_VIDEO);
    global.render.width = width;
    global.render.height = height;
    initialize_SDL("Engine", global.render.width, global.render.height);

    //Set flags
    global.render_flags = flags;
}

void render_begin(void){
    SDL_RenderClear(global.render.renderer);
}

void render_end(void){
    //Updates bitmap and writes it to a texture.
    if(bitmap_initialized) {
        draw_pixel_buffer();
        SDL_RenderCopy(global.render.renderer, bitmap, NULL, NULL);
    }

    SDL_RenderPresent(global.render.renderer);
}
