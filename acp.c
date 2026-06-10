/**
 * @file miniproject.c
 * @brief Complete C implementation of a menu-driven 2D Graphics Editor.
 *
 * This program implements a grid-based 2D Graphics Editor using a 2D character array.
 * Canvas elements are initialized to '_' (underscore) and graphical elements are drawn
 * using '*' (asterisk).
 *
 * Supported Shapes:
 *  1. Line (Bresenham's Line Algorithm)
 *  2. Rectangle (Axis-aligned outline)
 *  3. Circle (Aspect-ratio corrected radial plotting)
 *  4. Triangle (General 3-vertex outline drawn with lines)
 *
 * Features:
 *  - Non-destructive vector list: Add, delete, and modify shapes dynamically.
 *  - Safe rendering: Automatic coordinate checking (clipping) preventing memory corruption.
 *  - Robust input sanitization: Custom input scanner to avoid infinite loops on invalid chars.
 *
 * Compilation command:
 *   gcc -Wall miniproject.c -o miniproject -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ROWS 25
#define COLS 80
#define MAX_OBJECTS 100

/* --- Data Types & Models --- */

/**
 * @brief ShapeType enumeration representing the shape kind.
 */
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

/**
 * @brief Line specific coordinates.
 */
typedef struct {
    int x1, y1;
    int x2, y2;
} LineParams;

/**
 * @brief Rectangle top-left location and sizes.
 */
typedef struct {
    int x, y;
    int width, height;
} RectParams;

/**
 * @brief Circle center and radius.
 */
typedef struct {
    int cx, cy;
    int radius;
} CircleParams;

/**
 * @brief General triangle vertices.
 */
typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleParams;

/**
 * @brief Main Shape object container with active flag and unique ID.
 */
typedef struct {
    int id;
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams triangle;
    } data;
    int active; /* 1 = active, 0 = deleted */
} ShapeObject;

/* --- Canvas and Object State Storage --- */
char canvas[ROWS][COLS];
ShapeObject objects[MAX_OBJECTS];
int object_count = 0;

/* --- Function Prototypes --- */
void initCanvas(void);
void displayCanvas(void);
void redrawCanvas(void);

void drawLine(int x1, int y1, int x2, int y2);
void drawRectangle(int x, int y, int width, int height);
void drawCircle(int cx, int cy, int radius);
void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);

int addObject(ShapeObject obj);
int deleteObject(int id);
int modifyObject(int id, ShapeObject new_obj);

int getIntInput(const char *prompt, int min_val, int max_val);
int listActiveShapes(void);
void handleAddShape(void);
void handleDeleteShape(void);
void handleModifyShape(void);

/* --- Main Loop --- */
int main(void) {
    // Disable stdout and stdin buffering for interactive terminals
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    // Canvas setup
    initCanvas();

    int choice = 0;
    while (choice != 5) {
        printf("\n==================================\n");
        printf("       2D GRAPHICS EDITOR         \n");
        printf("==================================\n");
        printf("1. Add Shape to Canvas\n");
        printf("2. Delete Shape from Canvas\n");
        printf("3. Modify Existing Shape\n");
        printf("4. Display Canvas\n");
        printf("5. Exit Program\n");
        printf("----------------------------------\n");

        choice = getIntInput("Select an option (1-5): ", 1, 5);

        switch (choice) {
            case 1:
                handleAddShape();
                break;
            case 2:
                handleDeleteShape();
                break;
            case 3:
                handleModifyShape();
                break;
            case 4:
                displayCanvas();
                break;
            case 5:
                printf("\nExiting 2D Graphics Editor. Goodbye!\n");
                break;
        }
    }

    return 0;
}

/* --- Graphics Operations implementation --- */

/**
 * @brief Fills the 2D canvas array with the default character '_'.
 */
void initCanvas(void) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            canvas[i][j] = '_';
        }
    }
}

/**
 * @brief Prints the canvas to standard output enclosed in a visual border.
 */
void displayCanvas(void) {
    // Print top border
    printf("+");
    for (int j = 0; j < COLS; j++) {
        printf("-");
    }
    printf("+\n");

    // Print rows
    for (int i = 0; i < ROWS; i++) {
        printf("|");
        for (int j = 0; j < COLS; j++) {
            printf("%c", canvas[i][j]);
        }
        printf("|\n");
    }

    // Print bottom border
    printf("+");
    for (int j = 0; j < COLS; j++) {
        printf("-");
    }
    printf("+\n");
}

/**
 * @brief Clears canvas and renders all active shapes in order.
 */
void redrawCanvas(void) {
    initCanvas();
    for (int i = 0; i < object_count; i++) {
        if (objects[i].active) {
            switch (objects[i].type) {
                case SHAPE_LINE:
                    drawLine(objects[i].data.line.x1, objects[i].data.line.y1,
                             objects[i].data.line.x2, objects[i].data.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    drawRectangle(objects[i].data.rect.x, objects[i].data.rect.y,
                                  objects[i].data.rect.width, objects[i].data.rect.height);
                    break;
                case SHAPE_CIRCLE:
                    drawCircle(objects[i].data.circle.cx, objects[i].data.circle.cy,
                               objects[i].data.circle.radius);
                    break;
                case SHAPE_TRIANGLE:
                    drawTriangle(objects[i].data.triangle.x1, objects[i].data.triangle.y1,
                                 objects[i].data.triangle.x2, objects[i].data.triangle.y2,
                                 objects[i].data.triangle.x3, objects[i].data.triangle.y3);
                    break;
            }
        }
    }
}

/**
 * @brief Draws a straight line using Bresenham's integer line algorithm.
 * Plots pixels safely using clipping logic.
 */
void drawLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Guard against memory bounds
        if (x1 >= 0 && x1 < COLS && y1 >= 0 && y1 < ROWS) {
            canvas[y1][x1] = '*';
        }

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

/**
 * @brief Draws a rectangular box boundary.
 */
void drawRectangle(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    // Top and bottom horizontal lines
    for (int i = 0; i < width; i++) {
        int px = x + i;
        if (px >= 0 && px < COLS) {
            if (y >= 0 && y < ROWS) {
                canvas[y][px] = '*';
            }
            int bottom_y = y + height - 1;
            if (bottom_y >= 0 && bottom_y < ROWS) {
                canvas[bottom_y][px] = '*';
            }
        }
    }

    // Left and right vertical lines
    for (int i = 0; i < height; i++) {
        int py = y + i;
        if (py >= 0 && py < ROWS) {
            if (x >= 0 && x < COLS) {
                canvas[py][x] = '*';
            }
            int right_x = x + width - 1;
            if (right_x >= 0 && right_x < COLS) {
                canvas[py][right_x] = '*';
            }
        }
    }
}

/**
 * @brief Plots a circle with an aspect-ratio scaling parameter.
 */
void drawCircle(int cx, int cy, int radius) {
    if (radius < 0) {
        return;
    }
    if (radius == 0) {
        if (cx >= 0 && cx < COLS && cy >= 0 && cy < ROWS) {
            canvas[cy][cx] = '*';
        }
        return;
    }

    // Standard aspect ratio for terminal windows: column cells are narrower than row height.
    double aspect_ratio = 0.6;

    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            double dx = (x - cx) * aspect_ratio;
            double dy = (y - cy);
            double dist = sqrt(dx * dx + dy * dy);

            // Bounds check inside terminal scaling threshold
            if (fabs(dist - radius) < 0.5) {
                canvas[y][x] = '*';
            }
        }
    }
}

/**
 * @brief Draws a triangle connecting three vertices with Bresenham's lines.
 */
void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x3, y3);
    drawLine(x3, y3, x1, y1);
}

/* --- Database Operations implementation --- */

/**
 * @brief Appends a shape to the collection.
 * @return shape identifier if successful, 0 otherwise.
 */
int addObject(ShapeObject obj) {
    if (object_count >= MAX_OBJECTS) {
        return 0;
    }
    obj.id = object_count + 1;
    obj.active = 1;
    objects[object_count] = obj;
    object_count++;

    redrawCanvas();
    return obj.id;
}

/**
 * @brief Marks a shape in the collection as inactive.
 * @return 1 if found and marked, 0 if not found.
 */
int deleteObject(int id) {
    for (int i = 0; i < object_count; i++) {
        if (objects[i].id == id && objects[i].active) {
            objects[i].active = 0;
            redrawCanvas();
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Updates shape fields for a given ID.
 * @return 1 on success, 0 on failure.
 */
int modifyObject(int id, ShapeObject new_obj) {
    for (int i = 0; i < object_count; i++) {
        if (objects[i].id == id && objects[i].active) {
            new_obj.id = id;
            new_obj.active = 1;
            objects[i] = new_obj;
            redrawCanvas();
            return 1;
        }
    }
    return 0;
}

/* --- UI Logic implementation --- */

/**
 * @brief Safely reads a numeric input from the user, clearing error buffers.
 */
int getIntInput(const char *prompt, int min_val, int max_val) {
    int val;
    char term;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d%c", &val, &term) == 2 && (term == '\n' || term == ' ' || term == '\t')) {
            if (val >= min_val && val <= max_val) {
                return val;
            } else {
                printf("Error: Input out of allowed range [%d, %d]. Please try again.\n", min_val, max_val);
            }
        } else {
            printf("Error: Invalid integer format. Please try again.\n");
            // Drain standard input until newline
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }
}

/**
 * @brief Displays a summary of active objects.
 * @return Active objects count.
 */
int listActiveShapes(void) {
    int active_count = 0;
    printf("\n--- Active Shapes ---\n");
    for (int i = 0; i < object_count; i++) {
        if (objects[i].active) {
            active_count++;
            printf("ID [%d] : ", objects[i].id);
            switch (objects[i].type) {
                case SHAPE_LINE:
                    printf("Line from (%d, %d) to (%d, %d)\n",
                           objects[i].data.line.x1, objects[i].data.line.y1,
                           objects[i].data.line.x2, objects[i].data.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    printf("Rectangle at top-left (%d, %d), Width: %d, Height: %d\n",
                           objects[i].data.rect.x, objects[i].data.rect.y,
                           objects[i].data.rect.width, objects[i].data.rect.height);
                    break;
                case SHAPE_CIRCLE:
                    printf("Circle centered at (%d, %d) with Radius: %d\n",
                           objects[i].data.circle.cx, objects[i].data.circle.cy,
                           objects[i].data.circle.radius);
                    break;
                case SHAPE_TRIANGLE:
                    printf("Triangle with Vertices: A(%d, %d), B(%d, %d), C(%d, %d)\n",
                           objects[i].data.triangle.x1, objects[i].data.triangle.y1,
                           objects[i].data.triangle.x2, objects[i].data.triangle.y2,
                           objects[i].data.triangle.x3, objects[i].data.triangle.y3);
                    break;
            }
        }
    }
    if (active_count == 0) {
        printf("(No active shapes on canvas)\n");
    }
    printf("---------------------\n");
    return active_count;
}

/**
 * @brief Prompts user for shape choices and dimensions.
 */
void handleAddShape(void) {
    printf("\n--- Add a New Shape ---\n");
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    printf("5. Go Back\n");
    printf("-----------------------\n");

    int type_choice = getIntInput("Select shape type (1-5): ", 1, 5);
    if (type_choice == 5) return;

    ShapeObject obj;
    obj.type = (ShapeType)(type_choice - 1);

    switch (obj.type) {
        case SHAPE_LINE:
            printf("\nDefine Line coordinates (Canvas limits: columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.line.x1 = getIntInput("  Start X (column): ", 0, COLS - 1);
            obj.data.line.y1 = getIntInput("  Start Y (row): ", 0, ROWS - 1);
            obj.data.line.x2 = getIntInput("  End X (column): ", 0, COLS - 1);
            obj.data.line.y2 = getIntInput("  End Y (row): ", 0, ROWS - 1);
            break;

        case SHAPE_RECTANGLE:
            printf("\nDefine Rectangle coordinates (Canvas limits: columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.rect.x = getIntInput("  Top-left X (column): ", 0, COLS - 1);
            obj.data.rect.y = getIntInput("  Top-left Y (row): ", 0, ROWS - 1);
            obj.data.rect.width = getIntInput("  Width (cells): ", 1, COLS);
            obj.data.rect.height = getIntInput("  Height (cells): ", 1, ROWS);
            break;

        case SHAPE_CIRCLE:
            printf("\nDefine Circle coordinates:\n");
            obj.data.circle.cx = getIntInput("  Center X (column, 0-79): ", 0, COLS - 1);
            obj.data.circle.cy = getIntInput("  Center Y (row, 0-24): ", 0, ROWS - 1);
            obj.data.circle.radius = getIntInput("  Radius (cells): ", 0, 80);
            break;

        case SHAPE_TRIANGLE:
            printf("\nDefine Triangle coordinates (Three vertices, columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.triangle.x1 = getIntInput("  Vertex 1 X: ", 0, COLS - 1);
            obj.data.triangle.y1 = getIntInput("  Vertex 1 Y: ", 0, ROWS - 1);
            obj.data.triangle.x2 = getIntInput("  Vertex 2 X: ", 0, COLS - 1);
            obj.data.triangle.y2 = getIntInput("  Vertex 2 Y: ", 0, ROWS - 1);
            obj.data.triangle.x3 = getIntInput("  Vertex 3 X: ", 0, COLS - 1);
            obj.data.triangle.y3 = getIntInput("  Vertex 3 Y: ", 0, ROWS - 1);
            break;
    }

    int new_id = addObject(obj);
    if (new_id > 0) {
        printf("Success: Shape added successfully with ID [%d]!\n", new_id);
    } else {
        printf("Error: Failed to add shape (maximum capacity reached).\n");
    }
}

/**
 * @brief Prompts user for a shape ID to delete.
 */
void handleDeleteShape(void) {
    int active_count = listActiveShapes();
    if (active_count == 0) {
        return;
    }

    int id_to_delete = getIntInput("Enter the ID of the shape to delete: ", 1, object_count);

    if (deleteObject(id_to_delete)) {
        printf("Success: Shape ID [%d] deleted successfully!\n", id_to_delete);
    } else {
        printf("Error: Active shape with ID [%d] not found.\n", id_to_delete);
    }
}

/**
 * @brief Prompts user for a shape ID to modify, and inputs new shape parameters.
 */
void handleModifyShape(void) {
    int active_count = listActiveShapes();
    if (active_count == 0) {
        return;
    }

    int id_to_modify = getIntInput("Enter the ID of the shape to modify: ", 1, object_count);

    // Verify if shape exists and is active
    int found = 0;
    for (int i = 0; i < object_count; i++) {
        if (objects[i].id == id_to_modify && objects[i].active) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Error: Active shape with ID [%d] not found.\n", id_to_modify);
        return;
    }

    printf("\n--- Modify Shape [%d] ---\n", id_to_modify);
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    printf("5. Go Back\n");
    printf("-----------------------\n");

    int type_choice = getIntInput("Select new shape type (1-5): ", 1, 5);
    if (type_choice == 5) return;

    ShapeObject obj;
    obj.type = (ShapeType)(type_choice - 1);

    switch (obj.type) {
        case SHAPE_LINE:
            printf("\nDefine new Line coordinates (limits: columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.line.x1 = getIntInput("  Start X (column): ", 0, COLS - 1);
            obj.data.line.y1 = getIntInput("  Start Y (row): ", 0, ROWS - 1);
            obj.data.line.x2 = getIntInput("  End X (column): ", 0, COLS - 1);
            obj.data.line.y2 = getIntInput("  End Y (row): ", 0, ROWS - 1);
            break;

        case SHAPE_RECTANGLE:
            printf("\nDefine new Rectangle coordinates (limits: columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.rect.x = getIntInput("  Top-left X (column): ", 0, COLS - 1);
            obj.data.rect.y = getIntInput("  Top-left Y (row): ", 0, ROWS - 1);
            obj.data.rect.width = getIntInput("  Width: ", 1, COLS);
            obj.data.rect.height = getIntInput("  Height: ", 1, ROWS);
            break;

        case SHAPE_CIRCLE:
            printf("\nDefine new Circle coordinates:\n");
            obj.data.circle.cx = getIntInput("  Center X (column, 0-79): ", 0, COLS - 1);
            obj.data.circle.cy = getIntInput("  Center Y (row, 0-24): ", 0, ROWS - 1);
            obj.data.circle.radius = getIntInput("  Radius (cells): ", 0, 80);
            break;

        case SHAPE_TRIANGLE:
            printf("\nDefine new Triangle coordinates (Three vertices, columns 0-%d, rows 0-%d):\n", COLS - 1, ROWS - 1);
            obj.data.triangle.x1 = getIntInput("  Vertex 1 X: ", 0, COLS - 1);
            obj.data.triangle.y1 = getIntInput("  Vertex 1 Y: ", 0, ROWS - 1);
            obj.data.triangle.x2 = getIntInput("  Vertex 2 X: ", 0, COLS - 1);
            obj.data.triangle.y2 = getIntInput("  Vertex 2 Y: ", 0, ROWS - 1);
            obj.data.triangle.x3 = getIntInput("  Vertex 3 X: ", 0, COLS - 1);
            obj.data.triangle.y3 = getIntInput("  Vertex 3 Y: ", 0, ROWS - 1);
            break;
    }

    if (modifyObject(id_to_modify, obj)) {
        printf("Success: Shape ID [%d] modified successfully!\n", id_to_modify);
    } else {
        printf("Error: Failed to modify shape.\n");
    }
}