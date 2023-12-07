#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "raylib.h"
#include "raymath.h"

#define WIDTH 1920
#define HEIGHT 1080


struct MESH{
    float x1, x2, y1, y2, z1, z2;
};

struct OBST{
    float x1, x2, y1, y2, z1, z2;
};

struct HOLE{
    float x1, x2, y1, y2, z1, z2;
};

void parse_FDS(const std::string &filePath, std::vector<MESH> &mesh_vec, std::vector<OBST> &obst_vec, std::vector<HOLE> &hole_vec);
template <typename T> void print_FDS(std::vector<T> &vec, const std::string &type);
inline void print_vec(Vector3 vec){std::cout << "x=" << vec.x << ", y=" << vec.y << ", z=" << vec.z << std::endl;}
template <typename T> Vector3 model_centre(const std::vector<T> *vec);
template <typename T> float model_length(char axis, const std::vector<T> *vec);

void render_FDS(const std::vector<MESH> &mesh_vec, const std::vector<OBST> &obst_vec, const std::vector<HOLE> &hole_vec);

void mouse_cam_input(bool *user_camera_input, Camera *camera, int *camera_mode);
void mouse_input();
void keyboard_input();
void camera_change(int *camera_mode);

int main(){
    const std::string fds_filepath = "test2.fds";

    std::vector<MESH> mesh_vec;
    std::vector<OBST> obst_vec;
    std::vector<HOLE> hole_vec;
    parse_FDS(fds_filepath, mesh_vec, obst_vec, hole_vec);
    // print_FDS(mesh_vec, "MESH");
    // print_FDS(obst_vec, "OBST");
    // print_FDS(hole_vec, "HOLE");

    SetTraceLogLevel(7);                                        // LOG_ALL: 0, LOG_TRACE: 1, LOG_DEBUG: 2, LOG_INFO: 3, LOG_WARNING: 4, LOG_ERROR: 5, LOG_FATAL: 6, LOG_NONE: 7, 
    InitWindow(WIDTH, HEIGHT, "FDS Renderer");
    
    EnableCursor();                   
    SetTargetFPS(60);                   
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    Camera3D camera = {0};
    camera.position = (Vector3){model_centre(&mesh_vec).x, model_centre(&mesh_vec).y + 10.0f, model_centre(&mesh_vec).z + 10.0f};
    camera.target = (Vector3){model_centre(&mesh_vec).x, model_centre(&mesh_vec).y, model_centre(&mesh_vec).z }; // point to look at
    camera.up = (Vector3){0.0f, 0.0f, 1.0f}; // up direction
    camera.fovy = 100.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    int camera_mode = CAMERA_THIRD_PERSON;

    // Vector2 prev_mousepos = GetMousePosition();
    // Vector2 current_mousepos = GetMousePosition();
    bool user_camera_input = false;

    // print_vec(model_centre(&mesh_vec));

    while (!WindowShouldClose()){
        
        if (user_camera_input){
            UpdateCamera(&camera, camera_mode);
        }

        mouse_cam_input(&user_camera_input, &camera, &camera_mode);

        BeginDrawing();
        
        ClearBackground(WHITE);
        
        BeginMode3D(camera);

        render_FDS(mesh_vec, obst_vec, hole_vec);

        EndMode3D();

        EndDrawing();

    }

    CloseWindow();

    return 0;
}





// TODO: implement hole parsing
void parse_FDS(const std::string &filePath, std::vector<MESH> &mesh_vec, std::vector<OBST> &obst_vec, std::vector<HOLE> &hole_vec){
    std::ifstream file(filePath);
    if (!file.is_open()){
        std::cerr << "Error opening file: " << filePath << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    int ignore;
    while (std::getline(file, line)){
        MESH mesh;
        OBST obst;
        HOLE hole;
        if (sscanf(line.c_str(), "&MESH IJK= %d, %d, %d, XB= %f, %f, %f, %f, %f, %f / %*s", &ignore, &ignore, &ignore, &mesh.x1, &mesh.x2, &mesh.y1, &mesh.y2, &mesh.z1, &mesh.z2) == 9){
            mesh_vec.push_back(mesh);
        }
        else if (sscanf(line.c_str(), "&OBST XB=%f, %f, %f, %f, %f, %f", &obst.x1, &obst.x2, &obst.y1, &obst.y2, &obst.z1, &obst.z2) == 6){
            obst_vec.push_back(obst);
        }
    }

    file.close();
}

template <typename T>
void print_FDS(std::vector<T> &vec, const std::string &type){
    if (!vec.empty()){
        for (size_t i = 0; i < vec.size(); ++i){
            const T &element = vec[i];
            std::cout << type << " " << i + 1 << ": "
                      << "x1=" << element.x1 << ", x2=" << element.x2
                      << ", y1=" << element.y1 << ", y2=" << element.y2
                      << ", z1=" << element.z1 << ", z2=" << element.z2 << std::endl;
        }
    } 
    else {std::cout << type << " vec empty" << std::endl;}
}

template <typename T>
Vector3 model_centre(const std::vector<T>* vec){
    if (vec->empty()) {
        return {0.0f, 0.0f, 0.0f};
    }

    float min_x = vec->at(0).x1;
    float min_y = vec->at(0).y1;
    float min_z = vec->at(0).z1;
    float max_x = vec->at(0).x2;
    float max_y = vec->at(0).y2;
    float max_z = vec->at(0).z2;

    for (const auto &element : *vec) {
        min_x = fmin(min_x, element.x1);
        min_y = fmin(min_y, element.y1);
        min_z = fmin(min_z, element.z1);
        max_x = fmax(max_x, element.x2);
        max_y = fmax(max_y, element.y2);
        max_z = fmax(max_z, element.z2);
    }

    float centre_x = (min_x + max_x) / 2.0f;
    float centre_y = (min_y + max_y) / 2.0f;
    float centre_z = (min_z + max_z) / 2.0f;

    return {centre_x, centre_y, centre_z};
}

template <typename T>
float model_length(char axis, const std::vector<T> *vec){
    if (vec->empty()) {
        return 0.0f;
    }

    float min_axis;
    float max_axis;
    switch (axis){
        case 'x':
            min_axis = vec->at(0).x1;
            max_axis = vec->at(0).x2;
            for (const auto &obst : *vec) {
                min_axis = fmin(min_axis, obst.x1);
                max_axis = fmax(max_axis, obst.x2);
            }
            break;

        case 'y':
            min_axis = vec->at(0).y1;
            max_axis = vec->at(0).y2;
            for (const auto &obst : *vec) {
                min_axis = fmin(min_axis, obst.y1);
                max_axis = fmax(max_axis, obst.y2);
            }
            break;
        
        case 'z':
            min_axis = vec->at(0).z1;
            max_axis = vec->at(0).z2;
            for (const auto &obst : *vec) {
                min_axis = fmin(min_axis, obst.z1);
                max_axis = fmax(max_axis, obst.z2);
            }
            break;
    }
    
    return max_axis - min_axis;
}

// TODO: either mesh or obst is shifted
// TODO: fix camera on mouse button up
void render_FDS(const std::vector<MESH> &mesh_vec, const std::vector<OBST> &obst_vec, const std::vector<HOLE> &hole_vec){
    for (const OBST &obst : obst_vec){
            Vector3 position = {(obst.x1 + obst.x2) / 2, (obst.y1 + obst.y2) / 2, (obst.z1 + obst.z2) / 2};
            Vector3 size = {obst.x2 - obst.x1, obst.y2 - obst.y1, obst.z2 - obst.z1};

            DrawCubeWires(position, size.x, size.y, size.z, BLACK);
            DrawCube(position, size.x, size.y, size.z, GRAY);
        }

    for (const MESH &mesh : mesh_vec){
        Vector3 position = {(mesh.x1 + mesh.x2) / 2, (mesh.y1 + mesh.y2) / 2, (mesh.z1 + mesh.z2) / 2};
        Vector3 size = {mesh.x2 - mesh.x1, mesh.y2 - mesh.y1, mesh.z2 - mesh.z1};

        DrawCubeWires(position, size.x, size.y, size.z, BLUE);
    }
}

void mouse_cam_input(bool *user_camera_input, Camera *camera, int *camera_mode){
    float rotation_speed = 0.2f;
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)){
        DisableCursor();
        *user_camera_input = true;
        *camera_mode = CAMERA_THIRD_PERSON;
        camera->target.y = Clamp(camera->target.y, -89.0f, 89.0f);
    }
    Camera cameraa = *camera;       // dereference otherwise wierd scroll issue with -> opperator
    int scroll = GetMouseWheelMove();
    if (scroll != 0){
        DisableCursor();
        *user_camera_input = true;
        float zoom_speed = 50.0f;
        cameraa.fovy += static_cast<float>(scroll) * zoom_speed;
        cameraa.fovy = Clamp(cameraa.fovy, 1.0f, 180.0f);
    }
    // else {
    //     EnableCursor();
    //     *user_camera_input = false;
    //     // *camera_mode = CAMERA_FREE;
    // }      
}


void keyboard_input(){
    // if (IsKeyDown(KEY_SPACE)){
        // camera_mode = CAMERA_FREE;
        // // camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // reset
        // }
        // if (IsKeyDown(MOUSE_BUTTON_MIDDLE)){
        //     camera_mode = CAMERA_THIRD_PERSON;
        //     // camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // reset
        // }
}

void camera_change(){
    // if (camera_mode == 1)
    //     DrawText(TextFormat("Camera Mode: %s", "Free Camera"), 10, HEIGHT - 20, 10, BLACK);
    // if (camera_mode == 2)
    //     DrawText(TextFormat("Camera Mode: %s", "Orbtal"), 10, HEIGHT - 20, 10, BLACK);
    // if (camera_mode == 3)
    //     DrawText(TextFormat("Camera Mode: %s", "First Person"), 10, HEIGHT - 20, 10, BLACK);
    // if (camera_mode == 4)
    //     DrawText(TextFormat("Camera Mode: %s", "Third Person"), 10, HEIGHT - 20, 10, BLACK);
}