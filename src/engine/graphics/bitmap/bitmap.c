#include <SDL.h>
#include "../../../../include/util.h"
#include "../../../../include/global.h"
#include "../../../../include/graphics/bitmap.h"
#include "pixel_threading.h"

static SDL_Color** pixel_buffer = NULL; //2D array that is initialized to be a 2D matrix of pixels that is HEIGHT, WIDTH (rows and cols)
static u32 bitmap_scale_x = 1; //Field for appropriately transform the bitmap to fit the screen width (X values)
static u32 bitmap_scale_y = 1; //Field for appropriately transform the bitmap to fit the screen height (Y values)
SDL_Texture* bitmap = NULL; //Bitmap for drawing pixels to screen
bool bitmap_initialized = false; //Field to tell if we've initialized bitmap graphics

void set_background_clear(SDL_Color* color){
    if(color != NULL) {
        if (SDL_SetRenderDrawColor(global.render.renderer, color->r, color->g, color->b, color->a) != 0) {
            ERROR_EXIT("Failed to set default render graphics color.\n")
        }
    }
    else if(SDL_SetRenderDrawColor(global.render.renderer, 0, 0, 0, 0) != 0) {
        ERROR_EXIT("Failed to set default render graphics color.\n")
    }
}

void set_stroke_fill(SDL_Color* color){
    global.bitmap.stroke_fill.r = color->r;
    global.bitmap.stroke_fill.g = color->g;
    global.bitmap.stroke_fill.b = color->b;
}

void set_shape_fill(SDL_Color* color){
    global.bitmap.shape_fill.r = color->r;
    global.bitmap.shape_fill.g = color->g;
    global.bitmap.shape_fill.b = color->b;
}

void set_bitmap_tint(SDL_Color* color){
    global.bitmap.tint.r = color->r;
    global.bitmap.tint.g = color->g;
    global.bitmap.tint.b = color->b;
}

void fill_background(SDL_Color *color) {
    for(i32 i = 0; i < global.bitmap.height; i++){
        for(i32 j = 0; j < global.bitmap.width; j++){
            draw_pixel(color, j, i);
        }
    }
}

static i32 clamp(i32 value, i32 min, i32 max){
    if(value < min)
        value = min;
    else if(value > max)
        value = max;
    return value;
}

void draw_pixel(SDL_Color* color, i32 x, i32 y){
    if(pixel_buffer == NULL){
        ERROR_EXIT("Pixel Buffer not initialized, make sure that you have enabled the bitmap with the Render Flag BITMAP_ENABLED!")
    }

    if(x < 0 || x >= global.bitmap.width || y < 0 || y >= global.bitmap.height)
        return;

    //Update the pixel buffer, this does not graphics to the screen just yet, we use draw_pixel_buffer() to do that.
    if(color != NULL && *(u32*)&pixel_buffer[y][x] != *(u32*)color){
        pixel_buffer[y][x] = *color;
    }
}

void draw_pixels_from_surface(SDL_Surface* surface){
    u32* current_row;

    for(int y = 0; y < surface->h; y++){
        current_row = (u32 *)((u8 *) surface->pixels + y * surface->pitch);
        for(int x = 0; x < surface->w; x++){
            draw_pixel((SDL_Color*)&current_row[x], x, y);
        }
    }
}

void rotate_bitmap_deg(u32 degrees) {
    global.bitmap.rotation = degrees % 360;
}

void rotate_bitmap_rad(f64 radians) {
    global.bitmap.rotation = (u32)(radians * (180.0 / M_PI)) % 360;
}

void bitmap_shift(i32 x, i32 y) {
    global.bitmap.transform.x = x;
    global.bitmap.transform.y = y;
}

void bitmap_scale(u32 width_scale, u32 height_scale) {
    if(width_scale > 500)
        width_scale = 500;
    if(height_scale > 500)
        height_scale = 500;

    if(width_scale == 0 || height_scale == 0){
        WARN("Scale-factor of 0 percent not allowed!\n")
        return;
    }

    f32 scale_w = 100.0f/width_scale;
    f32 scale_h = 100.0f/height_scale;

    global.bitmap.transform.w = global.render.width * scale_w;
    global.bitmap.transform.h = global.render.height * scale_h;
}

void initialize_bitmap(u32 width, u32 height){
    if(!bitmap_initialized) {
        Bitmap_State bmp;
        bmp.width = global.render.width;
        bmp.height = global.render.height;
        bmp.rotation = 0;

        bmp.tint.r = 255;
        bmp.tint.g = 255;
        bmp.tint.b = 255;

        bmp.stroke_fill.r = 0;
        bmp.stroke_fill.g = 0;
        bmp.stroke_fill.b = 0;

        bmp.shape_fill.r = 255;
        bmp.shape_fill.g = 255;
        bmp.shape_fill.b = 255;

        bmp.transform.x = 0;
        bmp.transform.y = 0;
        bmp.transform.w = global.render.width;
        bmp.transform.h = global.render.height;

        if(width > 0)
            bmp.width = width;
        if(height > 0)
            bmp.height = height;

        global.bitmap = bmp; //Copies the bitmap state information to global

        //Sets the scale-factor to be the largest, strictly less-than integer multiple of the with and height
        //This is so that we can repeat this many pixels on the X,Y axes and THEN use a interpolation algorithm to scale the bitmap for larger resolutions.
        bitmap_scale_x = floor(global.render.width / global.bitmap.width);
        bitmap_scale_y = floor(global.render.height / global.bitmap.height);

        if ((pixel_buffer = (SDL_Color**)calloc(global.bitmap.height, sizeof(SDL_Color*))) == NULL) {
            ERROR_EXIT("Failed to initialize pixel buffer rows.\n")
        }

        for (int i = 0; i < global.bitmap.height; i++) {
            pixel_buffer[i] = (SDL_Color*)calloc(global.bitmap.width, sizeof(SDL_Color));

            if (pixel_buffer[i] == NULL) {
                ERROR_EXIT("Failed to initialize pixel buffer columns.\n")
            }
        }

        bitmap = SDL_CreateTexture(global.render.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, global.bitmap.width * bitmap_scale_x, global.bitmap.height * bitmap_scale_y);

        if(bitmap == NULL){
            ERROR_EXIT("Failed to initialize bitmap matrix (Your window size might be too small).\n")
        }

        bitmap_initialized = true;
    }
    else {
        WARN("Repeated usage of initialize_bitmap() is disallowed and undefined, remove any repeated references to this function.")
    }
}

void draw_pixel_buffer(){
    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32); //Format (really want this to be static...)
    void* pixels; //Read-Only Pixels read from the image to update (flattened 2D array).
    u32* current_row; //Current row of pixels we're writing to.
    i32 pitch; //Length of one row in bytes...I have no idea why it's called pitch.

    //Need to reset the texture in case the window size changed.
    bitmap_scale_x = global.render.width / global.bitmap.width;
    bitmap_scale_y = global.render.height / global.bitmap.height;

    SDL_DestroyTexture(bitmap);
    bitmap = SDL_CreateTexture(global.render.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, global.bitmap.width * bitmap_scale_x, global.bitmap.height * bitmap_scale_y);

    if(SDL_LockTexture(bitmap, NULL, &pixels, &pitch) != 0){
        //Just exits and retries to render this the next frame.
        //We do this to just avoid a crash and this only happens if the window size changes rapidly.
        return;
    }

    //The reason why we need to update all the pixels is that it is not a safe assumption to say that
    //the read in pixels are going to be initialized with the current screen colors, SDL does not guarantee this!

    //u32 rather than SDL_Color is used because the texture interprets the bits as colors correctly.
    //This transform is using a Kronecker's Product between our bitmap matrix and our rescaled matrix that is scale_x and scale_y larger.

    //Multithreaded version, should basically always be used.
    if(global.render_flags & MULTITHREADING_ENABLED){
        process_pixels(pixels, pixel_buffer, format, global.bitmap.width, global.bitmap.height, pitch, bitmap_scale_x, bitmap_scale_y);
    }
    else {
        for (int i = 0; i < global.bitmap.height; i++) {
            for (int j = 0; j < global.bitmap.width; j++) {
                u32 color = SDL_MapRGBA(format,
                                        pixel_buffer[i][j].r,
                                        pixel_buffer[i][j].g,
                                        pixel_buffer[i][j].b,
                                        pixel_buffer[i][j].a);

                //Rescaling the bitmap pixels to fit a larger resolution
                for (int y = 0; y < bitmap_scale_y; y++) {
                    current_row = (u32 *) ((u8 *) pixels + (i * bitmap_scale_y + y) * pitch);
                    for (int x = 0; x < bitmap_scale_x; x++) {
                        current_row[j * bitmap_scale_x + x] = color;
                    }
                }

            }
        }
    }

    SDL_UnlockTexture(bitmap);
    SDL_FreeFormat(format);
}

