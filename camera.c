#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/buttons.h"
#include "lib/colors.h"
#include "lib/device.h"
#include "lib/display.h"
#include "lib/fonts/fonts.h"
#include "lib/image.h"
#include "lib/log.h"
#include "lib/camera.h"
#include "lib/client.h"

#define VIEWER_FOLDER "viewer/"
#define MAX_ENTRIES 8
#define MAX_TEXT_SIZE 400
#define MAX_FILE_NAME 100
#define MAX_EXTENSION_SIZE 10

// Colors — Feel free to change these to fit your preference
#define BACKGROUND_COLOR WHITE
#define FONT_COLOR BLACK
#define SELECTED_BG_COLOR BYU_BLUE
#define SELECTED_FONT_COLOR BYU_LIGHT_SAND

// Makes sure to deinitialize everything before program close
void intHandler(int dummy) {
    log_info("Exiting...");
    display_exit();
    exit(0);
}

uint8_t getFileExtension(char fileName[], char *fileExtension) {
    log_debug("Beginning getFileExtension");
    char *currExtension = fileExtension;
    for (uint16_t j = 0; j < MAX_EXTENSION_SIZE; j++) {
        currExtension[j] = 0;
    }
    uint8_t currExtensionSize = 0;

    for (uint16_t i = 0; fileName[i] != 0; i++) {
        char message[64] = "";
        sprintf(message, "Current char: %c", fileName[i]);
        log_debug(message);

        switch (fileName[i]) {

        case 0:
            break;

        case '.':
            currExtensionSize = 0;
            for (uint16_t j = 0; j < MAX_EXTENSION_SIZE; j++) {
                currExtension[j] = 0;
            }
            break;

        default:
            currExtension[currExtensionSize] = fileName[i];
            currExtensionSize++;
        }
    }

    return currExtensionSize;
}
/*
 * Takes in a folder, reads the contents of the folder, filtering out any files that do not end with
 * .log or .bmp. This function should check to make sure the folder exists. It fills in the entries
 * array with all of the entries in the folder, up to 8 (MAX_ENTRIES). The function returns the
 * number of entries it put into the entries array.
 */
int get_entries(char *folder, char entries[][MAX_FILE_NAME]) {
    log_info("starting to get entries");
        //Looking at Dir
        DIR *dp;
        dp = opendir(folder);

    if (dp == NULL) {
        intHandler(1);
    }

    int numEntries = 0;
    log_info("Iterating through entries");
    log_set_level(LOG_INFO);
    
    log_debug("Declaring the dirent");
    struct dirent *dir;

    log_debug("Setting the dirent to the first pointer");
    dir = readdir(dp);//read what im pointing at
    if (dir == NULL) {
        intHandler(2);
    }

    log_debug("Starting the loop");

    log_set_level(LOG_INFO);

   // int numIterations = 0;
    while ((dir != NULL) && numEntries < MAX_ENTRIES) {
        //char message[164];//DEB
        char *currName = dir->d_name;
        //sprintf(message, "Iteration %d: %s", numIterations++, currName);
        //log_debug(message);

        log_debug("extracting file extension");
        char currExt[MAX_EXTENSION_SIZE] = "dummy";
        uint8_t currExtLength = getFileExtension(currName, currExt);

        if (currExtLength == 0) {
            log_debug("No file extension");
            dir = readdir(dp); //read it again 
            continue;
        } else if (strcmp(currExt, "bmp") == 0) {
            log_debug(".bmp");
            memcpy(entries[numEntries], currName, strlen(currName) + 1);
            numEntries++;
        } else if (strcmp(currExt, "log") == 0) {
            log_debug(".log");
            memcpy(entries[numEntries], currName, strlen(currName) + 1);
            numEntries++;
        } else {
            log_debug("extension provided not supported");
        }

        dir = readdir(dp);
    }

    closedir(dp);
    log_info("Closing directory");
    return numEntries;
    return -1;
}

/*
 * Draws the menu of the screen. It uses the entries array to create the menu, with the num_entries
 * specifying how many entries are in the entries array. The selected parameter is the item in the
 * menu that is selected and should be highlighted. Use BACKGROUND_COLOR, FONT_COLOR,
 * SELECTED_BG_COLOR, and SELECTED_FONT_COLOR to help specify the colors of the background, font,
 * select bar color, and selected text color.
 */
void draw_menu(char entries[][MAX_FILE_NAME], int num_entries, int selected) { 
    display_clear(BACKGROUND_COLOR);
    for (int i = 0; i < num_entries; i++) {
        uint16_t txtColor = (i == selected) ? SELECTED_FONT_COLOR : FONT_COLOR;
        uint16_t bkgColor = (i == selected) ? SELECTED_BG_COLOR : BACKGROUND_COLOR;
        display_draw_rectangle(0, (14 * i), DISPLAY_WIDTH, 15 + (14 * i), bkgColor, true, 1);
        display_draw_string(4, 2 + (14 * i), entries[i], &Font8, bkgColor, txtColor);
    }
}

/*
 * Displays an image or a log file. This function detects the type of file that should be draw. If
 * it is a bmp file, then it calls display_draw_image. If it is a log file, it opens the file, reads
 * 100 characters (MAX_TEXT_SIZE), and displays the text using display_draw_string. Combine folder
 * and file_name to get the complete file path.
 */
// Draws a .log file
void draw_log(char *folder, char *file_name) {
    display_clear(BLACK);
    char fileRoot[256] = "dummy";
    sprintf(fileRoot, "%s%s", folder, file_name);

    FILE *fp;
    fp = fopen(fileRoot, "r");

    char data[MAX_TEXT_SIZE];
    fread(data, 1, MAX_TEXT_SIZE, fp);
    fclose(fp);

    display_draw_string(4, 2, data, &Font8, BLACK, SELECTED_FONT_COLOR);
}

// Draws a .bmp file
void draw_bmp(char *folder, char *file_name) {
    display_clear(WHITE);
    char fileRoot[256] = "dummy";
    sprintf(fileRoot, "%s%s", folder, file_name);
    display_draw_image(fileRoot);
}

void draw_file(char *folder, char *file_name) {
    char currExt[MAX_EXTENSION_SIZE] = "dummy";
    getFileExtension(file_name, currExt);

    if (strcmp(currExt, "bmp") == 0) {
        draw_bmp(folder, file_name);
    } else if (strcmp(currExt, "log") == 0) {
        draw_log(folder, file_name);
    } else {
        log_debug("extension provided not supported");
    }

    delay_ms(2000);
}

/*
* TAKING A PICTURE
* this is the new content for this lab
* when the center button is pressed, a picture should be taken with the camera module.
* it should then display the image on the screen, and a new menu should take over the buttons.
* This menu contains the functions from the image lab (color channel removal, or_filter)
* it should loop through these options on button presses (and reset the image between) until
* the center button is pushed again.
* Note that many students will put their code directly in the main loop - Tristan was kind enough
* to put all of his code into functions. This is the best way to do it, as eventually things will
* shift around. Encourage students to functionize code blocks.
* 
* see also camera.c and camera.h, as those also have to be written by students.
*/
void picture_menu() {
    log_info("calling picture menu");
    display_clear(BLACK);
    display_draw_string(5, 5, "Smile!", &Font16, BLACK, DARK_GREEN);

    for (uint8_t i = 3; i > 0; i--) {
        display_draw_char(60, 60, i + 48, &Font20, BLACK, WHITE);
        delay_ms(1000);
    }

    display_clear(BLACK);
    display_draw_string(5, 5, "Saving picture", &Font16, BLACK, DARK_GREEN);

    log_set_level(LOG_DEBUG);
    uint8_t *picture_buffer = malloc(sizeof(uint8_t) * IMG_SIZE);
    log_debug("Allocated memory");

    camera_capture_data(picture_buffer, IMG_SIZE);
    log_debug("Saving the picture");

    Bitmap *bmp_pointer = malloc(sizeof(Bitmap));
    log_debug("Initialized the picture pointer");

    log_debug("Creating the bmp");
    create_bmp(bmp_pointer, picture_buffer);
    camera_save_to_file(picture_buffer, IMG_SIZE, "viewer/doorbe.bmp");
    free(picture_buffer);

    display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);

    log_info("Entering picture loop");
    // note that somewhere in the code there has to be a picture reset call. 
    // Tristan's calls are inside his filter functions.
    // if you don't do that, you'll destroy the image...
    while (true) {
        delay_ms(200);
         if (button_down()==0) {
            or_filter(bmp_pointer);
             reset_pixel_data(bmp_pointer);
             display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);
                 log_info("pressinf down");delay_ms(200);
        } if (button_left()==0) {
            remove_color_channel(BLUE_CHANNEL, bmp_pointer);
            display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);
            reset_pixel_data(bmp_pointer);delay_ms(200);
        } if (button_up()== 0) {
            remove_color_channel(GREEN_CHANNEL, bmp_pointer);
            display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);
            reset_pixel_data(bmp_pointer);delay_ms(200);
        } if (button_right()==0) {
            remove_color_channel(RED_CHANNEL, bmp_pointer);
            display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);
            reset_pixel_data(bmp_pointer);delay_ms(200);
        } if (button_center ()==0){
            break;
        }
        display_draw_image_data(get_pxl_data(bmp_pointer), DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
    // this stuff is very important... if you don't free things it will only work once.
    // another common error is to free it too early, and then it has no data to display
    destroy_bmp(bmp_pointer);
    free(bmp_pointer);
    log_set_level(LOG_INFO);
    log_info("Leaving picture menu");

}

int main(void) {

    signal(SIGINT, intHandler);

    log_info("Starting...");

    // THESE LINES MUST BE HERE, IN THIS ORDER, BEFORE
    // YOU USE THE BUTTONS OR THE DISPLAY
    display_init();
    buttons_init();

    delay_ms(200); 
    char entries[MAX_ENTRIES][MAX_FILE_NAME];
    int numEntries = get_entries(VIEWER_FOLDER, entries);
    // Everything inside this loop will repeat until 'Ctrl-C' is pressed in the terminal.
    int selected = 0;
    
    draw_menu(entries, numEntries, selected);
    while (true) {
        //delay rapid fire changes
        delay_ms(100);

        if (button_up() == 0) {
            // Do something upon detecting button press
            // This statement makes sure that selected stays in the range [0, NUM_ENTRIES)
            // The "+ NUM_ENTRIES" makes sure selected never goes negative, because aparently
            // -1 % 5 = -1. 
            selected = (selected + numEntries - 1) % numEntries;
            draw_menu(entries, numEntries, selected);
            while (button_up() == 0) {
                // Delay while the button is pressed to avoid repeated actions
                delay_ms(1);
            }
        }

        // Implement other button logic here
        if (button_down() == 0) {
            selected = (selected + numEntries + 1) % numEntries;
            draw_menu(entries, numEntries, selected);
            while (button_down() == 0) { delay_ms(1); }
        }
        //RIGHT
        if (button_right() == 0) {
           draw_file(VIEWER_FOLDER, entries[selected]);
           draw_menu(entries, numEntries, selected);
        }
        if (button_center() == 0) {
            picture_menu();
           draw_file(VIEWER_FOLDER, entries[selected]);
           numEntries = get_entries(VIEWER_FOLDER, entries);
           draw_menu(entries, numEntries, selected);
        }
    } 
        
    return 0;
}
   
