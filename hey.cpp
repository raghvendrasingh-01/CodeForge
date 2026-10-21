#include <iostream>
using namespace std;

class ParkingFloor {
    int floorNumber,numberOfSlots;
    bool *slots;

public:
    ParkingFloor(int f, int n) {
        floorNumber = f;
        numberOfSlots = n;
        slots = new bool[n];

        for (int i = 0; i < n; i++)
            slots[i] = false;
    }
    void reserve(int position) {
        if (position >= 1 && position <= numberOfSlots) {
            if (!slots[position - 1]) {
                slots[position - 1] = true;
                cout << "Slot " << position << " reserved.\n";
            } else cout << "Slot already reserved.\n";
        } else {
            cout << "Invalid slot.\n";
        }
    }
    void reserve(int start, int count) {
        if (count <= 0 || start < 1 || start + count - 1 > numberOfSlots) {
            cout << "Invalid range.\n";
            return;
        }

        for(int i = start - 1; i < start - 1 + count; i++) {
            if (slots[i]) {
                cout << "Some slots are already reserved.\n";
                return;
            }
        }

        for (int i = start - 1; i < start - 1 + count; i++)
            slots[i] = true;

        cout << count << " slots reserved starting from " << start << ".\n";
    }
    void display() {
        cout << "Floor " << floorNumber << ": ";

        for (int i = 0; i < numberOfSlots; i++)
            cout << (slots[i] ? "[X] " : "[ ] ");

        cout << endl;
    }

    ~ParkingFloor() {
        delete[] slots;
    }
};

int main() {
    int floors, slots;

    cout << "Enter number of floors: ";
    cin >> floors;

    cout << "Enter number of slots per floor: ";
    cin >> slots;

    if (floors <= 0 || slots <= 0) {
        cout << "Invalid number of floors or slots.\n";
        return 1;
    }

    ParkingFloor **building = new ParkingFloor*[floors];

    for (int i = 0; i < floors; i++)
        building[i] = new ParkingFloor(i + 1, slots);

    building[0]->reserve(2);
    building[0]->reserve(4, 3);

    for (int i = 0; i < floors; i++)
        building[i]->display();

    for (int i = 0; i < floors; i++)
        delete building[i];

    delete[] building;

    return 0;
}
