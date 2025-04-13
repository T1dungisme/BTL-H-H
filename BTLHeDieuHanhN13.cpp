#include<bits/stdc++.h>
using namespace std;

// ========================================
// Biến toàn cục cho hệ thống phân trang
// ========================================
int pageSize;   // Kích thước trang (bytes)
int processSize; // Kích thước tiến trình (bytes)
int ramSize;    // tổng dung lượng RAM (bytes)
int allocatedFrames; // Số khung RAM được cấp phát cho tiến trình
int numPages;  // Số trang mà tiến trình sẽ được chia thành
int numFrames; // Tổng số khung có trong RAM

vector<int> pageTable; // Bảng trang: lưu khung RAM cho mỗi trang, page i nằm ở khung nào (nếu có), bằng -1 nếu chưa được cấp phát
vector<int> ram;       // Mảng đại diện trạng thái của các khung RAM: khung i chứa trang nào, -1 là khung trống
queue<int> fifoQueue;  // Hàng hỗ trợ thuật toán FIFO

//// Biến và mảng hỗ trợ thuật toán LRU
int currentTime = 0;                // Thời gian tăng dần theo mỗi lần truy cập trang. (dùng cho LRU)
vector<int> lastAccess;             // Lưu thời gian truy cập cuối cùng của từng khung RAM (size = allocatedFrames).

// biến đếm số lỗi trang
int pageFaults = 0; 

// ========================================
// Hàm khởi tạo hệ thống phân trang và nhập dữ liệu của từng trang
// ========================================
void initPaging() { 
    cout << "Nhap kich thuoc trang (bytes): ";
    cin >> pageSize;
    cout << "Nhap kich thuoc tien trinh (bytes): ";
    cin >> processSize;
    cout << "Nhap dung luong RAM (bytes): ";
    cin >> ramSize;
    cout << "Nhap so khung RAM duoc cap phat cho tien trinh: ";  
    cin >> allocatedFrames;

    numFrames = ramSize / pageSize; // Tính số khung RAM có trong hệ thống

    // Kiểm tra hợp lệ
    if (allocatedFrames > numFrames) {
        cout << "Loi: So khung RAM duoc cap phat (" << allocatedFrames 
             << ") lon hon tong so khung RAM co the su dung (" << numFrames << ").\n";
        exit(1);
    }
    
    // Tính số trang của tiến trình 
    numPages = processSize / pageSize + (processSize % pageSize != 0);
   

    // Khởi tạo bảng trang và mảng RAM.
    pageTable.assign(numPages, -1); // -1: Tất cả các trang chưa vào RAM
    ram.assign(allocatedFrames, -1); // -1: Tất cả các khung RAM cấp phát ban đầu trống

    // Khởi tạo mảng hỗ trợ LRU
    lastAccess.assign(allocatedFrames, 0); // Ban đầu là 0 cho mọi khung.

    cout << "\n== Khoi tao xong: " << numPages << " trang, " << allocatedFrames << " khung ==\n\n";
}

// ========================================
// Hàm in trạng thái hiện tại của RAM và bảng trang
// ========================================
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

// ========================================
// Các hàm thay thế trang theo các thuật toán khác nhau
// ========================================

// ----- Thuật toán FIFO -----
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

// Truy cập trang theo FIFO
void accessPage_FIFO(int page) {
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
            // Cấp phát nếu khung trống
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

// ----- Thuật toán LRU ----- (Least Recently Used)
void replacePage_LRU(int newPage) {
    // Tìm vị trí của khung có last access time nhỏ nhất (ít được sử dụng gần đây nhất)
    int minTime = INT_MAX;
    int frameToReplace = -1;
    for (int i = 0; i < allocatedFrames; ++i) {
        if (lastAccess[i] < minTime) {
            minTime = lastAccess[i];
            frameToReplace = i;
        }
    }

    // Tìm trang cũ đang lưu trong khung được chọn
    int oldPage = -1;
    for (int p = 0; p < numPages; ++p) {
        if (pageTable[p] == frameToReplace) {
            oldPage = p;
            break;
        }
    }

    // Thực hiện thay thế trang
    ram[frameToReplace] = newPage;
    if (oldPage != -1) {
        pageTable[oldPage] = -1; // Gỡ trang cũ khỏi bảng trang
    }
    pageTable[newPage] = frameToReplace;

    // Cập nhật thời gian truy cập cho khung vừa thay thế
    currentTime++;
    lastAccess[frameToReplace] = currentTime;

    cout << ">> Thay the trang (LRU): Trang " 
         << (oldPage == -1 ? -1 : oldPage) 
         << " bi thay boi trang " << newPage << " o khung " << frameToReplace << "\n";
}

// Hàm truy cập trang theo LRU
void accessPage_LRU(int page) {
    cout << "\nTruy cap trang " << page << " (LRU): ";

    // Nếu trang đã có trong RAM, cập nhật last access time
    if (pageTable[page] != -1) {
        int frame = pageTable[page];
        currentTime++;
        lastAccess[frame] = currentTime;
        cout << "Trang da nam trong RAM (khung " << frame << ").\n";
        return;
    }

    // Nếu chưa có trong RAM -> page fault
    cout << "LOI TRANG!\n";
    ++pageFaults;

    // Kiểm tra xem có khung trống không
    for (int i = 0; i < allocatedFrames; ++i) {
        if (ram[i] == -1) {
            ram[i] = page;
            pageTable[page] = i;
            currentTime++;
            lastAccess[i] = currentTime;
            cout << ">> Cap phat trang " << page << " vao khung " << i << "\n";
            return;
        }
    }

    // Nếu không còn khung trống, gọi hàm thay thế trang theo LRU
    replacePage_LRU(page);
}

// ----- Thuật toán Optimal -----
// Tìm trang sẽ không được sử dụng lâu nhất trong tương lai
// (hoặc không được sử dụng nữa) để thay thế
void replacePage_Optimal(int newPage, int currentIndex, const vector<int>& referenceList) { 

    int frameToReplace = -1; //Lưu chỉ số khung RAM sẽ được thay thế. Ban đầu được đặt -1
    int farthest = -1; //Lưu giá trị "thời gian" (chỉ số trong chuỗi tham chiếu)
    // của lần sử dụng sau cùng của trang ứng viên; ban đầu là -1.

    for (int i = 0; i < allocatedFrames; i++) {
        int candidatePage = ram[i]; // Trang ứng viên trong khung RAM i
        if (candidatePage == -1) { // Nếu khung RAM i trống, chọn nó để thay thế
            frameToReplace = i;
            break;
        }
        
        int nextUse = -1;     // Biến lưu vị trí (chỉ số) lần sử dụng tiếp theo của trang ứng viên trong chuỗi tham chiếu.

        // Duyệt từ vị trí currentIndex + 1 đến cuối chuỗi tham chiếu để xác định lần sử dụng tiếp theo của candidatePage.
        // Nếu tìm thấy candidatePage trong chuỗi tham chiếu, lưu vị trí đó vào nextUse.
        for (int j = currentIndex + 1; j < (int)referenceList.size(); j++) {
            if (referenceList[j] == candidatePage) {    
                nextUse = j;                                
                break;
            }
        }

        // Nếu sau khi duyệt mà nextUse vẫn là -1, nghĩa là trang ứng viên không được sử dụng lần nào trong tương lai.
        //Trong trường hợp đó, khung thứ i sẽ được chọn ngay để thay thế vì đây là lựa chọn tối ưu (không cần dùng nữa).
        if (nextUse == -1) {
            frameToReplace = i;
            break;
        }

        // Nếu nextUse lớn hơn farthest, nghĩa là trang ứng viên sẽ được sử dụng sau cùng trong tương lai.
        // Điều này đảm bảo rằng sau khi duyệt qua tất cả các khung, ta sẽ chọn khung chứa trang mà lần sử dụng 
        // tiếp theo xảy ra xa nhất (hoặc không xuất hiện nữa).
        if (nextUse > farthest) {
            farthest = nextUse;
            frameToReplace = i;
        }
    }
    
    // Tìm trang cũ đang lưu trong khung được chọn để thay thế
    int oldPage = -1;   //Ban đầu, giá trị của oldPage được khởi tạo là -1, nghĩa là chưa xác định trang nào đang chiếm khung.
    for (int p = 0; p < numPages; p++) {
        if (pageTable[p] == frameToReplace) {   
            oldPage = p;    // Nếu tìm thấy trang cũ trong frameToReplace, lưu chỉ số trang đó vào oldPage.
            break;
        }
    }
    //  
    ram[frameToReplace] = newPage; // Ghi trang mới vào khung RAM
    if(oldPage != -1) // Nếu oldPage khác -1, có nghĩa là đã tìm thấy trang cũ trong frameToReplace.
        pageTable[oldPage] = -1; // Gỡ trang cũ khỏi bảng trang
    pageTable[newPage] = frameToReplace; // Cập nhật bảng trang với trang mới
    
    cout << ">> Thay the trang (Optimal): Trang " 
         << (oldPage == -1 ? -1 : oldPage)
         << " bi thay boi trang " << newPage << " tai khung " << frameToReplace << "\n";
}

// Xử lý việc truy cập vào một trang cụ thể với thuật toán Optimal. Nếu trang đã có trong RAM (không gây lỗi trang),
// in ra thông báo. Nếu không, tiến hành cấp phát hoặc thay thế trang.
void accessPage_Optimal(int page, int currentIndex, const vector<int>& referenceList) {
    cout << "\nTruy cap trang " << page << " (Optimal): ";
    
    if (pageTable[page] != -1) {
        cout << "Trang da nam trong RAM (khung " << pageTable[page] << ").\n";
        return;
    }
    
    cout << "LOI TRANG!\n";
    ++pageFaults;
    
    for (int i = 0; i < allocatedFrames; i++) {
        if (ram[i] == -1) {
            ram[i] = page;
            pageTable[page] = i;
            cout << ">> Cap phat trang " << page << " vao khung " << i << "\n";
            return;
        }
    }
    
    replacePage_Optimal(page, currentIndex, referenceList);
}

// ========================================
// Hàm chuyển đổi địa chỉ logic sang địa chỉ vật lý
// ========================================
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
    // Bước 1: Khởi tạo hệ thống phân trang và nhập dữ liệu
    initPaging();

    // Cho người dùng chọn thuật toán thay thế trang
    cout << "\nChon thuat toan thay the trang:\n";
    cout << "1. FIFO\n2. LRU\n3. Optimal\nLua chon: ";
    int algoChoice;
    cin >> algoChoice;

    int n;
    cout << "Nhap so luong trang tham chieu: ";
    cin >> n;

    vector<int> referenceList(n);
    cout << "Nhap chuoi tham chieu (so trang, cach nhau boi dau cach):\n";
    for (int i = 0; i < n; ++i) {
        cin >> referenceList[i];
    }

    // Xử lý các truy cập trang theo thuật toán đã chọn
    for (int i = 0; i < n; i++) {
        int page = referenceList[i];
        if(algoChoice == 1) {
            accessPage_FIFO(page);
        } else if(algoChoice == 2) {
            accessPage_LRU(page);
        } else if(algoChoice == 3) {
            accessPage_Optimal(page, i, referenceList);
        } else {
            cout << "Lua chon thuat toan khong hop le!\n";
            return 1;
        }
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
// Kết thúc chương trình
// ========================================