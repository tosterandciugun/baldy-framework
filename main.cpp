#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <iostream>

using namespace std;

vector<string> students;
vector<string> selectedStudents;

HWND hButtonPick, hButtonImport, hButtonExport, hButtonWheel, hEdit, hwnd;

const int WHEEL_RADIUS = 150;
const int WHEEL_CENTER_X = 400;
const int WHEEL_CENTER_Y = 300;
const int ROTATION_SPEED = 2; // Degrees per frame

int currentRotation = 0;
int targetRotation = 0;
bool isSpinning = false;

void DrawWheel(HDC hdc) {
    if (students.empty()) {
        TextOut(hdc, WHEEL_CENTER_X - 50, WHEEL_CENTER_Y, "No students loaded", 17);
        return;
    }

    int numSegments = min(static_cast<size_t>(10), students.size()); // Limit to 10 segments for simplicity
    int segmentAngle = numSegments > 0 ? 360 / numSegments : 360;

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

    // Save original GDI objects
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

    for (int i = 0; i < numSegments; ++i) {
        int startAngle = i * segmentAngle;
        int endAngle = startAngle + segmentAngle;

        POINT pts[3] = {
            {WHEEL_CENTER_X, WHEEL_CENTER_Y},
            {WHEEL_CENTER_X + static_cast<int>(WHEEL_RADIUS * cos((startAngle + currentRotation) * M_PI / 180)),
             WHEEL_CENTER_Y - static_cast<int>(WHEEL_RADIUS * sin((startAngle + currentRotation) * M_PI / 180))},
            {WHEEL_CENTER_X + static_cast<int>(WHEEL_RADIUS * cos((endAngle + currentRotation) * M_PI / 180)),
             WHEEL_CENTER_Y - static_cast<int>(WHEEL_RADIUS * sin((endAngle + currentRotation) * M_PI / 180))}
        };

        Polygon(hdc, pts, 3);

        // Draw student name
        string student = students[i % students.size()]; // Safe access
        if (!student.empty()) {
            TextOut(hdc,
                    WHEEL_CENTER_X + static_cast<int>(WHEEL_RADIUS * cos((startAngle + endAngle + currentRotation) / 2 * M_PI / 180)) - 50,
                    WHEEL_CENTER_Y - static_cast<int>(WHEEL_RADIUS * sin((startAngle + endAngle + currentRotation) / 2 * M_PI / 180)) - 10,
                    student.c_str(), student.size());
        }
    }

    // Restore original GDI objects and clean up
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void SpinWheel() {
    if (!isSpinning || students.empty()) {
        return;
    }

    currentRotation += ROTATION_SPEED;
    if (currentRotation >= targetRotation) {
        isSpinning = false;
        int numSegments = min(static_cast<size_t>(10), students.size());
        int segmentAngle = numSegments > 0 ? 360 / numSegments : 360;
        int selectedIndex = ((targetRotation % 360) / segmentAngle) % students.size();
        string selectedStudent = students[selectedIndex];
        SetWindowText(hEdit, selectedStudent.c_str());
        MessageBox(NULL, ("The wheel stopped at: " + selectedStudent).c_str(), "Wheel of Fortune", MB_OK | MB_ICONINFORMATION);
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

void PickStudent() {
    if (students.empty()) {
        MessageBox(NULL, "Please import a list of students first.", "No Students", MB_OK | MB_ICONWARNING);
        return;
    }

    if (selectedStudents.size() == students.size()) {
        MessageBox(NULL, "All students have been selected. Resetting the list.", "All Students Selected", MB_OK | MB_ICONINFORMATION);
        selectedStudents.clear();
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, students.size() - 1);
    int random_index;

    do {
        random_index = dis(gen);
    } while (find(selectedStudents.begin(), selectedStudents.end(), students[random_index]) != selectedStudents.end());

    string selectedStudent = students[random_index];
    selectedStudents.push_back(selectedStudent);
    MessageBox(NULL, ("Today, " + selectedStudent + " will present their homework.").c_str(), "Selected Student", MB_OK | MB_ICONINFORMATION);
    SetWindowText(hEdit, selectedStudent.c_str());
}

void ImportStudents() {
    OPENFILENAME ofn;
    char szFile[260];
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd; // Use hwnd for proper dialog ownership
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        ifstream file(ofn.lpstrFile);
        if (file.is_open()) {
            students.clear();
            selectedStudents.clear();
            string line;
            while (getline(file, line)) {
                if (!line.empty()) { // Skip empty lines
                    students.push_back(line);
                }
            }
            file.close();
            MessageBox(NULL, "Students imported successfully.", "Success", MB_OK | MB_ICONINFORMATION);
            InvalidateRect(hwnd, NULL, TRUE); // Redraw wheel
        } else {
            MessageBox(NULL, "Could not open file for reading.", "Error", MB_OK | MB_ICONWARNING);
        }
    }
}

void ExportStudents() {
    if (students.empty()) {
        MessageBox(NULL, "There are no students to export.", "No Students", MB_OK | MB_ICONWARNING);
        return;
    }

    OPENFILENAME ofn;
    char szFile[260];
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn) == TRUE) {
        ofstream file(ofn.lpstrFile);
        if (file.is_open()) {
            for (const string& student : students) {
                file << student << "\n";
            }
            file.close();
            MessageBox(NULL, "Students exported successfully.", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(NULL, "Could not open file for writing.", "Error", MB_OK | MB_ICONWARNING);
        }
    }
}

void StartSpin() {
    if (students.empty()) {
        MessageBox(NULL, "Please import a list of students first.", "No Students", MB_OK | MB_ICONWARNING);
        return;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 360 * 10);
    targetRotation = dis(gen);
    isSpinning = true;
    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                PickStudent();
            } else if (LOWORD(wParam) == 2) {
                ImportStudents();
            } else if (LOWORD(wParam) == 3) {
                ExportStudents();
            } else if (LOWORD(wParam) == 4) {
                StartSpin();
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawWheel(hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER: {
            SpinWheel();
            break;
        }
        case WM_DESTROY:
            KillTimer(hwnd, 1); // Clean up timer
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        CLASS_NAME,
        "Teacher's Student Picker",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    hButtonPick = CreateWindow("BUTTON", "Pick a Student", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, 150, 30, hwnd, (HMENU)1, hInstance, NULL);
    hButtonImport = CreateWindow("BUTTON", "Import Students", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 50, 150, 30, hwnd, (HMENU)2, hInstance, NULL);
    hButtonExport = CreateWindow("BUTTON", "Export Students", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 90, 150, 30, hwnd, (HMENU)3, hInstance, NULL);
    hButtonWheel = CreateWindow("BUTTON", "Spin Wheel", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 130, 150, 30, hwnd, (HMENU)4, hInstance, NULL);
    hEdit = CreateWindow("EDIT", "", WS_BORDER | WS_VISIBLE | WS_CHILD, 170, 10, 200, 30, hwnd, NULL, hInstance, NULL);

    if (hButtonPick == NULL || hButtonImport == NULL || hButtonExport == NULL || hButtonWheel == NULL || hEdit == NULL) {
        MessageBox(NULL, "Button or edit control creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    SetTimer(hwnd, 1, 20, NULL);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
