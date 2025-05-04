#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// ANSI color codes
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define RESET       "\x1b[0m"
#define RED         "\x1b[31m"
#define GREEN       "\x1b[32m"
#define YELLOW      "\x1b[33m"
#define CYAN        "\x1b[36m"
#define BROWN       "\x1b[33m"  // Brown via yellow
#define COLOR_RESET   "\033[0m"
#define COLOR_AQUA    "\033[1;36m"
#define COLOR_CORAL   "\033[1;38;5;209m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_BLUE    "\033[1;34m"

#define MAX_USERS 100
#define MAX_MENU_ITEMS 50
#define MAX_ORDERS 50
#define MAX_CATEGORIES 3
#define ITEMS_PER_CATEGORY 3

typedef struct {
    char username[50];
    char email[100];
    char phone[15];
    char password[50];
    char role[20]; // Admin, Customer, Chef
} User;

typedef struct {
    char name[50];
    char category[20];
    float price;
} MenuItem;

typedef struct {
    char customerName[50];
    char itemName[50];
    int quantity;
    char status[20]; // Processing, Ready, Delivered
    float totalAmount;
    time_t orderTime;
} Order;

typedef enum {
    CASH,
    BKASH,
    ROCKET,
    NAGAD,
    VISA,
    MASTERCARD
} PaymentMethod;

User users[MAX_USERS];
MenuItem menu[MAX_MENU_ITEMS];
Order orders[MAX_ORDERS];
int userCount = 0, menuCount = 0, orderCount = 0;

const char* USER_DB_FILE = "users.txt";
const char* ORDER_DB_FILE = "orders.txt";
const char* categories[MAX_CATEGORIES] = {"Bengali", "Pakistani", "Turkish"};

// Function prototypes
void initializeMenu();
void registerUser(char *role);
int loginUser(char *role, char *username);
void adminMenu(char *currentUsername);
void customerMenu(char *currentUsername);
void chefMenu(char *currentUsername);
void addMenuItem();
void editMenuItem();
void deleteMenuItem();
void viewMenu();
void placeOrder(char *currentUsername);
void viewOrders(char *currentUserRole, char *currentUsername);
void updateOrderStatus();
void processPayment(float total);
void hidePassword(char *password);
void saveUserToFile(User user);
void saveOrderToFile(Order order);
void loadOrdersFromFile();
bool isEmailValid(const char *email);
bool isPhoneValid(const char *phone);
bool isPasswordValid(const char *password);
void loadUsersFromFile();
bool isUsernameTaken(const char *username);
bool isEmailTaken(const char *email);
bool isPhoneTaken(const char *phone);
void saveAllUsersToFile();
void saveAllOrdersToFile();
int getNumericInput(int min, int max, const char *prompt);
void forgotPassword();
void generateOTP(char *otp);
void sendDemoOTP(const char *email, const char *otp);
void clearInputBuffer();
void viewCustomerOrderHistory();
void displayLogo();

void displayLogo() {
    printf(BROWN "=====================================\n");
    printf("||                                    ||\n");
    printf("||" GREEN "     RESTAURANT MANAGEMENT         " BROWN "||\n");
    printf("||" CYAN  "           SYSTEM                  " BROWN "||\n");
    printf("||                                   ||\n");
    printf("=====================================" RESET "\n");
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void initializeMenu() {
    // Bengali Items
    strcpy(menu[0].name, "Plain Rice");
    strcpy(menu[0].category, "Bengali");
    menu[0].price = 50.0;
    
    // Pakistani Items
    strcpy(menu[1].name, "Biryani");
    strcpy(menu[1].category, "Pakistani");
    menu[1].price = 180.0;
    
    // Turkish Items
    strcpy(menu[2].name, "Doner");
    strcpy(menu[2].category, "Turkish");
    menu[2].price = 200.0;
    
    menuCount = 3;
}

void loadOrdersFromFile() {
    FILE *file = fopen(ORDER_DB_FILE, "r");
    if (file == NULL) {
        return;
    }

    orderCount = 0;
    while (fscanf(file, "%49[^,],%49[^,],%d,%19[^,],%f,%ld\n", 
           orders[orderCount].customerName,
           orders[orderCount].itemName,
           &orders[orderCount].quantity,
           orders[orderCount].status,
           &orders[orderCount].totalAmount,
           &orders[orderCount].orderTime) == 6) {
        orderCount++;
        if (orderCount >= MAX_ORDERS) break;
    }
    fclose(file);
}

void saveOrderToFile(Order order) {
    FILE *file = fopen(ORDER_DB_FILE, "a");
    if (file == NULL) {
        printf(COLOR_RED "Error opening order database file!\n" COLOR_RESET);
        return;
    }

    fprintf(file, "%s,%s,%d,%s,%.2f,%ld\n", 
            order.customerName,
            order.itemName,
            order.quantity,
            order.status,
            order.totalAmount,
            order.orderTime);
    fclose(file);
}

void saveAllOrdersToFile() {
    FILE *file = fopen(ORDER_DB_FILE, "w");
    if (file == NULL) {
        printf(COLOR_RED "Error opening order database file!\n" COLOR_RESET);
        return;
    }

    for (int i = 0; i < orderCount; i++) {
        fprintf(file, "%s,%s,%d,%s,%.2f,%ld\n", 
                orders[i].customerName,
                orders[i].itemName,
                orders[i].quantity,
                orders[i].status,
                orders[i].totalAmount,
                orders[i].orderTime);
    }
    fclose(file);
}

void hidePassword(char *password) {
    char ch;
    int i = 0;
    memset(password, 0, 50);
    
#ifdef _WIN32
    while ((ch = _getch()) != '\r') {
        if (ch == '\b' && i > 0) {
            printf("\b \b");
            i--;
            password[i] = '\0';
        } else if (ch != '\b' && i < 49) {
            password[i++] = ch;
            printf("*");
        }
    }
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while ((ch = getchar()) != '\n' && i < 49) {
        if (ch == '\b' && i > 0) {
            printf("\b \b");
            i--;
            password[i] = '\0';
        } else if (ch != '\b') {
            password[i++] = ch;
            printf("*");
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    password[i] = '\0';
    printf("\n");
}

int getNumericInput(int min, int max, const char *prompt) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1 && value >= min && value <= max) {
            clearInputBuffer();
            return value;
        }
        printf(COLOR_RED "Invalid input! Please enter a number between %d and %d.\n" COLOR_RESET, min, max);
        clearInputBuffer();
    }
}

void loadUsersFromFile() {
    FILE *file = fopen(USER_DB_FILE, "r");
    if (file == NULL) {
        return;
    }

    userCount = 0;
    while (fscanf(file, "%49[^,],%99[^,],%14[^,],%49[^,],%19[^\n]\n", 
           users[userCount].username, 
           users[userCount].email, 
           users[userCount].phone, 
           users[userCount].password, 
           users[userCount].role) == 5) {
        userCount++;
        if (userCount >= MAX_USERS) break;
    }
    fclose(file);
}

void saveUserToFile(User user) {
    FILE *file = fopen(USER_DB_FILE, "a");
    if (file == NULL) {
        printf(COLOR_RED "Error opening user database file!\n" COLOR_RESET);
        return;
    }

    fprintf(file, "%s,%s,%s,%s,%s\n", 
            user.username, 
            user.email, 
            user.phone, 
            user.password, 
            user.role);
    fclose(file);
}

void saveAllUsersToFile() {
    FILE *file = fopen(USER_DB_FILE, "w");
    if (file == NULL) {
        printf(COLOR_RED "Error opening user database file!\n" COLOR_RESET);
        return;
    }

    for (int i = 0; i < userCount; i++) {
        fprintf(file, "%s,%s,%s,%s,%s\n", 
                users[i].username, 
                users[i].email, 
                users[i].phone, 
                users[i].password, 
                users[i].role);
    }
    fclose(file);
}

bool isUsernameTaken(const char *username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return true;
        }
    }
    return false;
}

bool isEmailTaken(const char *email) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].email, email) == 0) {
            return true;
        }
    }
    return false;
}

bool isPhoneTaken(const char *phone) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].phone, phone) == 0) {
            return true;
        }
    }
    return false;
}

int userExists(char *username, char *password, char *role) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            strcpy(role, users[i].role);
            return 1;
        }
    }
    return 0;
}

bool isEmailValid(const char *email) {
    int atCount = 0, dotCount = 0;
    int len = strlen(email);
    if (len < 5) return false;

    for (int i = 0; i < len; i++) {
        if (email[i] == '@') atCount++;
        if (email[i] == '.') dotCount++;
    }
    return (atCount == 1 && dotCount > 0);
}

bool isPhoneValid(const char *phone) {
    if (strlen(phone) != 11) return false;
    for (int i = 0; i < 11; i++) {
        if (!isdigit(phone[i])) return false;
    }
    return true;
}

bool isPasswordValid(const char *password) {
    bool hasDigit = false, hasSpecialChar = false;
    if (strlen(password) < 8) return false;
    for (int i = 0; i < strlen(password); i++) {
        if (isdigit(password[i])) hasDigit = true;
        if (ispunct(password[i])) hasSpecialChar = true;
    }
    return (hasDigit && hasSpecialChar);
}

void registerUser(char *role) {
    if (userCount >= MAX_USERS) {
        printf(COLOR_RED "User limit reached!\n" COLOR_RESET);
        return;
    }

    User newUser;
    
    while (1) {
        printf("Enter username: ");
        scanf("%49s", newUser.username);
        clearInputBuffer();
        if (isUsernameTaken(newUser.username)) {
            printf(COLOR_RED "Username already taken! Please choose another.\n" COLOR_RESET);
        } else {
            break;
        }
    }

    while (1) {
        printf("Enter email: ");
        scanf("%99s", newUser.email);
        clearInputBuffer();
        if (!isEmailValid(newUser.email)) {
            printf(COLOR_RED "Invalid email format!\n" COLOR_RESET);
        } else if (isEmailTaken(newUser.email)) {
            printf(COLOR_RED "Email already registered! Please use another email.\n" COLOR_RESET);
        } else {
            break;
        }
    }

    while (1) {
        printf("Enter phone (11 digits): ");
        scanf("%14s", newUser.phone);
        clearInputBuffer();
        if (!isPhoneValid(newUser.phone)) {
            printf(COLOR_RED "Phone number must be 11 digits!\n" COLOR_RESET);
        } else if (isPhoneTaken(newUser.phone)) {
            printf(COLOR_RED "Phone number already registered!\n" COLOR_RESET);
        } else {
            break;
        }
    }

    char password[50];
    while (1) {
        printf("Enter password: ");
        hidePassword(password);
        if (!isPasswordValid(password)) {
            printf(COLOR_RED "Password must be at least 8 characters long, contain a digit and a special character.\n" COLOR_RESET);
        } else {
            strcpy(newUser.password, password);
            break;
        }
    }

    strcpy(newUser.role, role);
    users[userCount++] = newUser;
    saveUserToFile(newUser);

    printf(COLOR_GREEN "Registration successful as %s!\n" COLOR_RESET, role);
}

void generateOTP(char *otp) {
    const char digits[] = "0123456789";
    srand(time(0));
    for (int i = 0; i < 6; i++) {
        otp[i] = digits[rand() % 10];
    }
    otp[6] = '\0';
}

void sendDemoOTP(const char *email, const char *otp) {
    printf(COLOR_BLUE "\nDemo Email Sent to: %s\n", email);
    printf("Subject: Password Reset OTP\n");
    printf("Message: Your OTP for password reset is: %s\n", otp);
    printf("(In a real system, this would be sent via email)\n\n" COLOR_RESET);
}

void forgotPassword() {
    char username[50], email[100], otp[7], userOTP[7];
    int found = 0;
    User *user = NULL;
    
    printf(COLOR_AQUA "\nForgot Password\n" COLOR_RESET);
    printf("Enter your username: ");
    scanf("%49s", username);
    clearInputBuffer();
    
    printf("Enter your registered email: ");
    scanf("%99s", email);
    clearInputBuffer();
    
    // Find user with matching username and email
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].email, email) == 0) {
            user = &users[i];
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf(COLOR_RED "No account found with that username and email combination.\n" COLOR_RESET);
        return;
    }
    
    // Generate and send OTP
    generateOTP(otp);
    sendDemoOTP(email, otp);
    
    printf("Enter the OTP sent to your email: ");
    scanf("%6s", userOTP);
    clearInputBuffer();
    
    if (strcmp(otp, userOTP) != 0) {
        printf(COLOR_RED "Invalid OTP. Password reset failed.\n" COLOR_RESET);
        return;
    }
    
    // Get new password
    char newPassword[50];
    while (1) {
        printf("Enter new password: ");
        hidePassword(newPassword);
        if (!isPasswordValid(newPassword)) {
            printf(COLOR_RED "Password must be at least 8 characters long, contain a digit and a special character.\n" COLOR_RESET);
        } else {
            break;
        }
    }
    
    // Confirm password
    char confirmPassword[50];
    printf("Confirm new password: ");
    hidePassword(confirmPassword);
    
    if (strcmp(newPassword, confirmPassword) != 0) {
        printf(COLOR_RED "Passwords do not match. Password reset failed.\n" COLOR_RESET);
        return;
    }
    
    // Update password
    strcpy(user->password, newPassword);
    saveAllUsersToFile();
    printf(COLOR_GREEN "Password reset successfully!\n" COLOR_RESET);
}

int loginUser(char *role, char *username) {
    char password[50] = {0};
    int attempts = 0;
    int choice;
    
    while (attempts < 3) {
        printf("Enter username: ");
        scanf("%49s", username);
        clearInputBuffer();
        
        printf("Enter password: ");
        hidePassword(password);
        
        if (userExists(username, password, role)) {
            return 1;
        }
        
        attempts++;
        printf(COLOR_RED "Login failed. Invalid username or password.\n" COLOR_RESET);
        
        if (attempts == 2) {
            printf("Forgot password? Press '1' to reset password: ");
            scanf("%d",&choice);
            clearInputBuffer();
            if (choice == 1) {
                forgotPassword();
            }
        }
    }
    
    printf(COLOR_RED "Too many failed attempts. Please try again later.\n" COLOR_RESET);
    return 0;
}

void viewCustomerOrderHistory() {
    if (orderCount == 0) {
        printf(COLOR_YELLOW "\nNo orders have been placed yet.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CORAL "\nCustomer Order History:\n" COLOR_RESET);
    printf("----------------------------------------------------------------------------------------\n");
    printf("Customer        Email                   Phone        Item            Quantity    Amount\n");
    printf("----------------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < orderCount; i++) {
        char email[100] = "N/A";
        char phone[15] = "N/A";
        
        for (int j = 0; j < userCount; j++) {
            if (strcmp(users[j].username, orders[i].customerName) == 0) {
                strcpy(email, users[j].email);
                strcpy(phone, users[j].phone);
                break;
            }
        }
        
        printf("%-15s %-24s %-12s %-15s %-11d %.2ftk\n", 
               orders[i].customerName, 
               email,
               phone,
               orders[i].itemName, 
               orders[i].quantity,
               orders[i].totalAmount);
    }
    printf("----------------------------------------------------------------------------------------\n");
}

void adminMenu(char *currentUsername) {
    int choice;
    while (1) {
        printf(COLOR_CORAL "\nAdmin Menu:\n" COLOR_RESET);
        printf("1. Add Menu Item\n2. Delete Menu Item\n");
        printf("3. View Menu\n4. View Orders\n5. View Customer Order History\n6. Logout\n");
        choice = getNumericInput(1, 6, "Enter your choice: ");

        switch (choice) {
            case 1: addMenuItem(); break;
            case 2: deleteMenuItem(); break;
            case 3: viewMenu(); break;
            case 4: viewOrders("Admin", currentUsername); break;
            case 5: viewCustomerOrderHistory(); break;
            case 6: return;
            default: printf(COLOR_RED "Invalid choice\n" COLOR_RESET);
        }
    }
}

void customerMenu(char *currentUsername) {
    int choice;
    while (1) {
        printf(COLOR_CORAL "\nCustomer Menu:\n" COLOR_RESET);
        printf("1. View Menu\n2. Place Order\n3. View Orders\n4. Logout\n");
        choice = getNumericInput(1, 4, "Enter your choice: ");

        switch (choice) {
            case 1: viewMenu(); break;
            case 2: placeOrder(currentUsername); break;
            case 3: viewOrders("Customer", currentUsername); break;
            case 4: return;
            default: printf(COLOR_RED "Invalid choice\n" COLOR_RESET);
        }
    }
}

void chefMenu(char *currentUsername) {
    int choice;
    while (1) {
        printf(COLOR_CORAL "\nChef Menu:\n" COLOR_RESET);
        printf("1. View Orders\n2. Update Order Status\n3. Logout\n");
        choice = getNumericInput(1, 3, "Enter your choice: ");

        switch (choice) {
            case 1: viewOrders("Chef", currentUsername); break;
            case 2: updateOrderStatus(); break;
            case 3: return;
            default: printf(COLOR_RED "Invalid choice\n" COLOR_RESET);
        }
    }
}

void addMenuItem() {
    if (menuCount >= MAX_MENU_ITEMS) {
        printf(COLOR_RED "Menu is full!\n" COLOR_RESET);
        return;
    }

    printf("Enter item name: ");
    char name[50];
    scanf("%49[^\n]", name);
    clearInputBuffer();
    
    printf("Select category:\n");
    for (int i = 0; i < MAX_CATEGORIES; i++) {
        printf("%d. %s\n", i+1, categories[i]);
    }
    int catChoice = getNumericInput(1, MAX_CATEGORIES, "Enter category number: ");
    
    printf("Enter item price: ");
    float price;
    scanf("%f", &price);
    clearInputBuffer();
    
    strcpy(menu[menuCount].name, name);
    strcpy(menu[menuCount].category, categories[catChoice-1]);
    menu[menuCount].price = price;
    
    menuCount++;
    printf(COLOR_GREEN "Menu item added successfully!\n" COLOR_RESET);
}

void deleteMenuItem() {
    viewMenu();
    if (menuCount == 0) return;
    
    int itemNum = getNumericInput(1, menuCount, "Enter item number to delete: ");
    
    for (int i = itemNum-1; i < menuCount - 1; i++) {
        menu[i] = menu[i + 1];
    }
    menuCount--;
    printf(COLOR_GREEN "Menu item deleted successfully!\n" COLOR_RESET);
}

void viewMenu() {
    printf(COLOR_CORAL "\nMenu Items:\n" COLOR_RESET);
    printf("--------------------------------------------------\n");
    printf("No.  Category     Item Name          Price\n");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < menuCount; i++) {
        printf("%-4d %-12s %-18s %.2ftk\n", i+1, menu[i].category, menu[i].name, menu[i].price);
    }
    printf("--------------------------------------------------\n");
}

void placeOrder(char *currentUsername) {
    viewMenu();
    if (menuCount == 0) {
        printf(COLOR_RED "No items available to order.\n" COLOR_RESET);
        return;
    }
    
    int itemNum = getNumericInput(1, menuCount, "Enter item number to order: ");
    
    int quantity = getNumericInput(1, 100, "Enter quantity: ");
    
    strcpy(orders[orderCount].customerName, currentUsername);
    strcpy(orders[orderCount].itemName, menu[itemNum-1].name);
    orders[orderCount].quantity = quantity;
    strcpy(orders[orderCount].status, "Processing");
    
    float total = quantity * menu[itemNum-1].price;
    orders[orderCount].totalAmount = total;
    orders[orderCount].orderTime = time(NULL);
    
    saveOrderToFile(orders[orderCount]);
    orderCount++;
    
    processPayment(total);
}

void viewOrders(char *currentUserRole, char *currentUsername) {
    printf(COLOR_CORAL "\nCurrent Orders:\n" COLOR_RESET);
    printf("--------------------------------------------------------------------\n");
    printf("No.  Customer        Item            Quantity    Status      Amount    Time\n");
    printf("--------------------------------------------------------------------\n");
    
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(currentUserRole, "Admin") == 0 || 
            strcmp(currentUserRole, "Chef") == 0 ||
            strcmp(orders[i].customerName, currentUsername) == 0) {
            
            char timeStr[20];
            struct tm *timeinfo = localtime(&orders[i].orderTime);
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
            
            printf("%-4d %-15s %-15s %-11d %-11s %.2ftk    %s\n", 
                   i+1,
                   orders[i].customerName, 
                   orders[i].itemName, 
                   orders[i].quantity, 
                   orders[i].status,
                   orders[i].totalAmount,
                   timeStr);
        }
    }
    printf("--------------------------------------------------------------------\n");
}

void updateOrderStatus() {
    viewOrders("Chef", "Chef");
    if (orderCount == 0) return;
    
    int orderNum = getNumericInput(1, orderCount, "Enter order number to update status: ");
    
    printf("Current status: %s\n", orders[orderNum-1].status);
    printf("Enter new status (Processing/Ready/Delivered): ");
    char status[20];
    scanf("%19s", status);
    clearInputBuffer();
    
    if (strcmp(status, "Processing") != 0 && strcmp(status, "Ready") != 0 && strcmp(status, "Delivered") != 0) {
        printf(COLOR_RED "Invalid status! Status remains unchanged.\n" COLOR_RESET);
        return;
    }
    
    strcpy(orders[orderNum-1].status, status);
    saveAllOrdersToFile();
    printf(COLOR_GREEN "Order status updated!\n" COLOR_RESET);
}

void processPayment(float total) {
    printf(COLOR_CORAL "\nPayment Options:\n" COLOR_RESET);
    printf("1. Cash\n2. BKash\n3. Rocket\n4. NAGAD\n5. VISA\n6. MASTERCARD\n");
    int choice = getNumericInput(1, 6, "Select payment method: ");

    switch (choice) {
        case 1:
            printf(COLOR_GREEN "Paid %.2f in Cash. Thank you!\n" COLOR_RESET, total);
            break;
            
        case 2: case 3: case 4: {
            const char *service = (choice == 2) ? "BKash" : (choice == 3) ? "Rocket" : "NAGAD";
            printf("\nProcessing payment via %s (%.2f)\n", service, total);
            
            char input[20];
            while(1) {
                printf("Enter mobile number (11 digits): ");
                scanf("%11s", input);
                clearInputBuffer();
                if(strlen(input) == 11 && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid mobile number! Must be 11 digits.\n" COLOR_RESET);
            }
            
            while(1) {
                printf("Enter security code (3 digits): ");
                hidePassword(input);
                if(strlen(input) == 3 && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid security code! Must be 3 digits.\n" COLOR_RESET);
            }
            
            while(1) {
                printf("Enter %s PIN (4-6 digits): ", service);
                hidePassword(input);
                if(strlen(input) >= 4 && strlen(input) <= 6 && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid PIN! Must be 4-6 digits.\n" COLOR_RESET);
            }
            
            printf(COLOR_GREEN "Payment of %.2f via %s successful!\n" COLOR_RESET, total, service);
            printf(COLOR_GREEN "------------ Order has been placed ----------\n" COLOR_RESET);
            break;
        }
            
        case 5: case 6: {
            const char *cardType = (choice == 5) ? "VISA" : "Mastercard";
            printf("\nProcessing payment via %s (%.2f)\n", cardType, total);
            
            char input[20];
            while(1) {
                printf("Enter %s card number (16 digits): ", cardType);
                scanf("%16s", input);
                clearInputBuffer();
                if(strlen(input) == 16 && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid card number! Must be 16 digits.\n" COLOR_RESET);
            }
            
            while(1) {
                printf("Enter verification code (3-4 digits): ");
                hidePassword(input);
                if((strlen(input) == 3 || strlen(input) == 4) && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid verification code! Must be 3-4 digits.\n" COLOR_RESET);
            }
            
            while(1) {
                printf("Enter card PIN (4 digits): ");
                hidePassword(input);
                if(strlen(input) == 4 && isdigit(input[0])) break;
                printf(COLOR_RED "Invalid PIN! Must be 4 digits.\n" COLOR_RESET);
            }
            
            printf(COLOR_GREEN "Payment of %.2f via %s successful!\n" COLOR_RESET, total, cardType);
            break;
        }
            
        default:
            printf(COLOR_RED "Invalid payment method. Defaulting to Cash.\n" COLOR_RESET);
            printf(COLOR_GREEN "Paid %.2f in Cash. Thank you!\n" COLOR_RESET, total);
    }
}


int main() {
    // Initialize Windows console for ANSI colors if on Windows
    
    #ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, mode);
    #endif

    initializeMenu();
    loadUsersFromFile();
    loadOrdersFromFile();
    
    displayLogo();
    
    while (1) {
        char selectedRole[20];
        printf(COLOR_AQUA "\nSelect your role:\n");
        printf("1. Admin\n2. Customer\n3. Chef/Kitchen Staff\n4. Exit\n" COLOR_RESET);
        int roleChoice = getNumericInput(1, 4, "Enter your choice: ");
        
        if (roleChoice == 4) {
            printf(COLOR_YELLOW "\nExiting the system. Goodbye!\n" COLOR_RESET);
            break;
        }
        
        switch (roleChoice) {
            case 1: strcpy(selectedRole, "Admin"); break;
            case 2: strcpy(selectedRole, "Customer"); break;
            case 3: strcpy(selectedRole, "Chef"); break;
            default:
                printf(COLOR_RED "Invalid choice. Please try again.\n" COLOR_RESET);
                continue;
        }
        
        while (1) {
            printf(COLOR_CORAL "\n1. Register\n2. Login\n3. Back to Role Selection\n" COLOR_RESET);
            int authChoice = getNumericInput(1, 3, "Enter your choice: ");
            
            if (authChoice == 3) break;
            
            if (authChoice == 1) {
                registerUser(selectedRole);
            } else if (authChoice == 2) {
                char role[20];
                char username[50];
                if (loginUser(role, username)) {
                    printf(COLOR_GREEN "\nLogin successful as %s!\n" COLOR_RESET, role);
                    
                    if (strcmp(role, "Admin") == 0) {
                        adminMenu(username);
                    } else if (strcmp(role, "Customer") == 0) {
                        customerMenu(username);
                    } else if (strcmp(role, "Chef") == 0) {
                        chefMenu(username);
                    }
                }
            }
        }
    }
    
    saveAllOrdersToFile();
    return 0;
}