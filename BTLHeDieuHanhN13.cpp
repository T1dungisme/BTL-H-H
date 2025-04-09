#include<bits/stdc++.h>
using namespace std;


int pageSize;   // Kích thước trang (bytes)
int processSize; // Kích thước tiến trình (bytes)
int ramSize;    // tổng dung lượng RAM (bytes)
int allocatedFrames; // Số khung RAM được cấp phát cho tiến trình
int numPages;  // Số trang mà tiến trình sẽ được chia thành
int numFrames; // Tổng số khung có trong RAM

vector<int> pageTable; // Bảng trang: lưu khung RAM cho mỗi trang, page i nằm ở khung nào (nếu có), bằng -1 nếu chưa được cấp phát
vector<int> ram;       // Trạng thái các khung RAM: khung i chứa trang nào, -1 là khung trống
queue<int> fifoQueue;  // Hàng đợi dùng cho FIFO

int pageFaults = 0; // biến đếm số lỗi trang

// Khởi tạo phân trang
void initPaging() { 
    cout << "Nhap kich thuoc trang (bytes): ";
    cin >> pageSize;
    cout << "Nhap kich thuoc tien trinh (bytes): ";
    cin >> processSize;
    cout << "Nhap dung luong RAM (bytes): ";
    cin >> ramSize;
    cout << "Nhap so khung RAM duoc cap phat cho tien trinh: ";  
    cin >> allocatedFrames;

    numPages = processSize / pageSize + (processSize % pageSize != 0);
    numFrames = ramSize / pageSize;

    pageTable.assign(numPages, -1); // -1: Tất cả các trang chưa vào RAM
    ram.assign(allocatedFrames, -1); // -1: Tất cả các khung RAM cấp phát ban đầu trống

    cout << "\n== Khoi tao xong: " << numPages << " trang, " << allocatedFrames << " khung ==\n\n";
}

// Hiển thị trạng thái RAM và bảng trang
void printStatus() {
    // In trạng thái khung RAM
    // In bảng trang: trang nào đang nằm trong khung nào, hay đang nằm trên đĩa (chưa cấp phát)
    cout << "\n--- Trang Thai RAM ---\n";
    for (int i = 0; i < allocatedFrames; ++i) {
        cout << "[ " << (ram[i] == -1 ? " " : to_string(ram[i])) << " ] ";
    }

    cout << "\n\n--- Bang Trang ---\n";
    cout << left << setw(10) << "Trang" << setw(10) << "Khung" << setw(15) << "Trang thai\n";
    for (int i = 0; i < numPages; ++i) {
        if (pageTable[i] == -1)
            cout << setw(10) << i << setw(10) << "-" << setw(15) << "Tren dia\n";
        else
            cout << setw(10) << i << setw(10) << pageTable[i] << setw(15) << "Trong RAM\n";
    }
    cout << endl;
}

// Hàm thay thế trang theo FIFO
void replacePage_FIFO(int newPage) {
    int oldPage = fifoQueue.front();
    fifoQueue.pop();

    // Tìm khung chứa trang cũ
    int frameToReplace = pageTable[oldPage]; // Khung RAM chứa trang cũ

    // Ghi đè trang mới vào đó
    ram[frameToReplace] = newPage; // Ghi trang mới vào khung RAM
    pageTable[oldPage] = -1;         // Gỡ trang cũ khỏi bảng trang
    pageTable[newPage] = frameToReplace; // Cập nhật bảng trang với trang mới
    fifoQueue.push(newPage); // Thêm trang mới vào hàng đợi FIFO

    cout << ">> Thay the trang (FIFO): Trang " << oldPage << " bi thay boi trang " << newPage << "\n";
}

// Truy cập trang
void accessPage(int page) {
    cout << "\nTruy cap trang " << page << ": ";

    if (pageTable[page] != -1) {
        cout << "Trang da nam trong RAM (khung " << pageTable[page] << ").\n";
        return;
    }

    // Page fault
    cout << "LOI TRANG!\n";
    ++pageFaults;

    // Kiểm tra RAM còn khung trống không
    for (int i = 0; i < allocatedFrames; ++i) {
        if (ram[i] == -1) {
            // Cấp phát khung trống này
            ram[i] = page;
            pageTable[page] = i;
            fifoQueue.push(page);
            cout << ">> Cap phat trang " << page << " vao khung " << i << "\n";
            return;
        }
    }

    // Không còn khung trống → thay trang
    replacePage_FIFO(page);
}

// Chuyển đổi địa chỉ logic sang địa chỉ vật lý
void translateLogicalAddress(int logicalAddress) {
    int page = logicalAddress / pageSize; // Tính trang từ địa chỉ logic
    int offset = logicalAddress % pageSize; // Tính offset trong trang

    cout << "\n>>> Dich dia chi logic " << logicalAddress << ": ";
    if (page >= numPages) {
        cout << "LOI! Dia chi logic vuot qua kich thuoc tien trinh.\n";
        return;
    }

    if (pageTable[page] == -1) {
        cout << "LOI TRANG! Trang " << page << " chua duoc nap vao RAM.\n";
        return;
    }

    int frame = pageTable[page]; // Khung RAM chứa trang này
    int physicalAddress = frame * pageSize + offset; // Tính địa chỉ vật lý từ khung và offset

    cout << "Trang " << page << ", offset " << offset << "-> Dia chi vat ly: " << physicalAddress << "\n";
}


int main() {
    initPaging();

    int n;
    cout << "Nhap so luong trang tham chieu: ";
    cin >> n;

    vector<int> referenceList(n);
    cout << "Nhap chuoi tham chieu (so trang, cach nhau boi dau cach):\n";
    for (int i = 0; i < n; ++i) {
        cin >> referenceList[i];
    }

    for (int page : referenceList) {
        accessPage(page);
        printStatus();
    }

    cout << "\n== Tong so loi trang: " << pageFaults << " ==\n";
    cout << "== Ti le loi trang: " << fixed << setprecision(2) << (double)pageFaults / n * 100 << "% ==\n";

    // Cho người dùng thử nhập địa chỉ logic
    char choice;
    do {
        int logicalAddress;
        cout << "\nBan co muon nhap dia chi logic de dich sang dia chi vat ly? (y/n): ";
        cin >> choice;
        if (choice == 'y' || choice == 'Y') { // Nếu người dùng chọn 'y' hoặc 'Y'
            cout << "Nhap dia chi logic: ";
            cin >> logicalAddress;
            translateLogicalAddress(logicalAddress);
        }
    } while (choice == 'y' || choice == 'Y'); // Lặp lại cho đến khi người dùng không muốn nữa
    return 0;
}
