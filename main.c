#include <windows.h>
#include <math.h>
#include <stdio.h>

#define MAX_POINTS 100
#define INTERPOLATION_STEPS 100

typedef struct {
    float x, y;
} Point;

Point points[MAX_POINTS];
int pointCount = 4;
int draggingIndex = -1;
float tProgress = 0.0f;
BOOL isAnimating = FALSE;
BOOL isAddingPoint = FALSE;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void SavePointsToFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "%d\n", pointCount);
        for (int i = 0; i < pointCount; i++) {
            fprintf(file, "%f %f\n", points[i].x, points[i].y);
        }
        fclose(file);
    }
}

void LoadPointsFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fscanf(file, "%d", &pointCount);
        for (int i = 0; i < pointCount; i++) {
            fscanf(file, "%f %f", &points[i].x, &points[i].y);
        }
        fclose(file);
    }
}

void CatmullRomSpline(Point *p, float t, Point *pos, Point *tangent) {
    int p1 = (int)t;
    float localT = t - p1;

    if (p1 >= pointCount - 1) {
        *pos = p[pointCount - 1];
        *tangent = (Point){1, 0};
        return;
    }

    int p0 = max(p1 - 1, 0);
    int p2 = min(p1 + 1, pointCount - 1);
    int p3 = min(p2 + 1, pointCount - 1);

    float t2 = localT * localT;
    float t3 = t2 * localT;

    float a0 = -0.5f * t3 + t2 - 0.5f * localT;
    float a1 =  1.5f * t3 - 2.5f * t2 + 1.0f;
    float a2 = -1.5f * t3 + 2.0f * t2 + 0.5f * localT;
    float a3 =  0.5f * t3 - 0.5f * t2;

    pos->x = a0 * p[p0].x + a1 * p[p1].x + a2 * p[p2].x + a3 * p[p3].x;
    pos->y = a0 * p[p0].y + a1 * p[p1].y + a2 * p[p2].y + a3 * p[p3].y;
}

void DrawSpline(HDC hdc) {
    Point prev, current;
    CatmullRomSpline(points, 0.0f, &prev, &(Point){0, 0});

    for (int i = 1; i <= INTERPOLATION_STEPS; i++) {
        float t = (float)i / INTERPOLATION_STEPS * (pointCount - 1);
        CatmullRomSpline(points, t, &current, &(Point){0, 0});
        MoveToEx(hdc, (int)prev.x, (int)prev.y, NULL);
        LineTo(hdc, (int)current.x, (int)current.y);
        prev = current;
    }
}

void DrawScene(HWND hwnd, HDC hdc) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 800, 600);
    SelectObject(memDC, memBitmap);
    RECT rect = {0, 0, 800, 600};
    FillRect(memDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    for (int i = 0; i < pointCount; i++) {
        Ellipse(memDC, points[i].x - 5, points[i].y - 5, points[i].x + 5, points[i].y + 5);
    }

    DrawSpline(memDC);

    if (isAnimating) {
        Point rectPos, tangent;
        CatmullRomSpline(points, tProgress, &rectPos, &tangent);
        Rectangle(memDC, rectPos.x - 10, rectPos.y - 10, rectPos.x + 10, rectPos.y + 10);
    }

    BitBlt(hdc, 0, 0, 800, 600, memDC, 0, 0, SRCCOPY);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void UpdateAnimation(HWND hwnd) {
    if (isAnimating) {
        tProgress += 0.02f;
        if (tProgress >= pointCount - 1) {
            isAnimating = FALSE;
            KillTimer(hwnd, 1);
        }
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void CreateMainMenu(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();

    AppendMenu(hFileMenu, MF_STRING, 1, "Exit");
    AppendMenu(hFileMenu, MF_STRING, 2, "Save");
    AppendMenu(hFileMenu, MF_STRING, 3, "Load");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");

    SetMenu(hwnd, hMenu);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SplineWindow";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "SplineWindow", "Catmull-Rom Spline", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    CreateMainMenu(hwnd);
    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HDC hdc;
    static PAINTSTRUCT ps;

    switch (uMsg) {
        case WM_CREATE:
            points[0] = (Point){100, 300};
            points[1] = (Point){300, 100};
            points[2] = (Point){500, 300};
            points[3] = (Point){700, 500};
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            DrawScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            if (isAddingPoint && pointCount < MAX_POINTS) {
                points[pointCount++] = (Point){x, y};
            } else {
                for (int i = 0; i < pointCount; i++) {
                    if (abs(points[i].x - x) < 10 && abs(points[i].y - y) < 10) {
                        draggingIndex = i;
                        break;
                    }
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        } break;

        case WM_MOUSEMOVE:
            if (draggingIndex != -1) {
                points[draggingIndex].x = LOWORD(lParam);
                points[draggingIndex].y = HIWORD(lParam);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case WM_LBUTTONUP:
            draggingIndex = -1;
            break;

        case WM_KEYDOWN:
            if (wParam == VK_SPACE && !isAnimating) {
                tProgress = 0.0f;
                isAnimating = TRUE;
                SetTimer(hwnd, 1, 16, NULL);
            } else if (wParam == VK_TAB) {
                isAddingPoint = !isAddingPoint;
                SetWindowText(hwnd, isAddingPoint ? "Catmull-Rom Spline [ADD MODE]" : "Catmull-Rom Spline");
            }
            break;

        case WM_TIMER:
            UpdateAnimation(hwnd);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) PostQuitMessage(0);
            else if (LOWORD(wParam) == 2) SavePointsToFile("spline.dat");
            else if (LOWORD(wParam) == 3) LoadPointsFromFile("spline.dat");
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}