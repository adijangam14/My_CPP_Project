#include <iostream>
#include <string>
#include <vector>
#include <oci.h> // Oracle Call Interface
#include <cstring> // For strlen
#include <iomanip> // For setw  
using namespace std;

// Utility for handling OCI errors
void checkOCIStatus(sword status, OCIError* err) {
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
        char errBuf[512];
        sb4 errCode;
        OCIErrorGet(err, 1, nullptr, &errCode, (OraText*)errBuf, sizeof(errBuf), OCI_HTYPE_ERROR);
        cerr << "Error: " << errBuf << endl;
        exit(EXIT_FAILURE);
    }
}

// Struct to handle database connection and operations
struct Database {
    OCIEnv* env;
    OCIServer* server;
    OCISession* session;
    OCIError* err;
    OCISvcCtx* svc;
    OCIStmt* stmt;

    Database() : env(nullptr), server(nullptr), session(nullptr), err(nullptr), svc(nullptr) {}

    void connect() {
    cout << "Connecting to Oracle database..." << endl;

    // Initialize OCI Environment
    checkOCIStatus(OCIEnvCreate(&env, OCI_THREADED | OCI_OBJECT, nullptr, nullptr, nullptr, nullptr, 0, nullptr), err);
    checkOCIStatus(OCIHandleAlloc(env, (void**)&err, OCI_HTYPE_ERROR, 0, nullptr), err);
    checkOCIStatus(OCIHandleAlloc(env, (void**)&server, OCI_HTYPE_SERVER, 0, nullptr), err);

    // Database credentials
    const char* username = "sys";        // Use sys user
    const char* password = "415712";      // Use your password
    const char* db_service_name = "localhost:1521/XEPDB1"; // Adjust to match your DB service name

    // Attach to the server
    checkOCIStatus(OCIServerAttach(server, err, (const OraText*)db_service_name, strlen(db_service_name), OCI_DEFAULT), err);
    checkOCIStatus(OCIHandleAlloc(env, (void**)&svc, OCI_HTYPE_SVCCTX, 0, nullptr), err);
    checkOCIStatus(OCIAttrSet(svc, OCI_HTYPE_SVCCTX, server, 0, OCI_ATTR_SERVER, err), err);

    // Allocate session handle and set credentials
    checkOCIStatus(OCIHandleAlloc(env, (void**)&session, OCI_HTYPE_SESSION, 0, nullptr), err);
    checkOCIStatus(OCIAttrSet(session, OCI_HTYPE_SESSION, (void*)username, strlen(username), OCI_ATTR_USERNAME, err), err);
    checkOCIStatus(OCIAttrSet(session, OCI_HTYPE_SESSION, (void*)password, strlen(password), OCI_ATTR_PASSWORD, err), err);

    // Start the session as SYSDBA by passing OCI_SYSDBA instead of OCI_DEFAULT
    checkOCIStatus(OCISessionBegin(svc, err, session, OCI_CRED_RDBMS, OCI_SYSDBA), err);
    checkOCIStatus(OCIAttrSet(svc, OCI_HTYPE_SVCCTX, session, 0, OCI_ATTR_SESSION, err), err);

    cout << "Connected to Oracle database successfully as SYSDBA." << endl;
    }

    void disconnect() {
        if (session) {
            OCISessionEnd(svc, err, session, OCI_DEFAULT);
            OCIHandleFree(session, OCI_HTYPE_SESSION);
        }
        if (server) {
            OCIServerDetach(server, err, OCI_DEFAULT);
            OCIHandleFree(server, OCI_HTYPE_SERVER);
        }
        if (svc) OCIHandleFree(svc, OCI_HTYPE_SVCCTX);
        if (err) OCIHandleFree(err, OCI_HTYPE_ERROR);
        if (env) OCIHandleFree(env, OCI_HTYPE_ENV);
    }

    ~Database() {
        disconnect();
    }

    void executeSQL(const string& sql) {
    OCIStmt* stmt = nullptr;
    try {
        checkOCIStatus(OCIHandleAlloc(env, (void**)&stmt, OCI_HTYPE_STMT, 0, nullptr), err);

        checkOCIStatus(OCIStmtPrepare(stmt, err, 
                      (const OraText*)sql.c_str(), 
                      (ub4)sql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT), err);

        checkOCIStatus(OCIStmtExecute(svc, stmt, err, 
                      1, 0, nullptr, nullptr, OCI_DEFAULT), err);

        cout << "SQL executed successfully." << endl;
    } catch (...) {
        cerr << "Error executing SQL statement." << endl;
    }

    // Cleanup
    if (stmt) OCIHandleFree(stmt, OCI_HTYPE_STMT);
}

void checkAndCreateTables() {
    cout << "Checking and creating tables if necessary..." << endl;

    // SQL statements to create tables safely
    string create_books = 
        "BEGIN "
        "   EXECUTE IMMEDIATE 'CREATE TABLE Books ("
        "       book_id INT PRIMARY KEY, "
        "       title VARCHAR2(256), "
        "       author VARCHAR2(256), "
        "       available_copies INT)'; "
        "EXCEPTION "
        "   WHEN OTHERS THEN "
        "       IF SQLCODE != -955 THEN "
        "           RAISE; "
        "       END IF; "
        "END;";

    string create_users = 
        "BEGIN "
        "   EXECUTE IMMEDIATE 'CREATE TABLE Users ("
        "       user_id INT PRIMARY KEY, "
        "       name VARCHAR2(256), "
        "       contact_info VARCHAR2(256))'; "
        "EXCEPTION "
        "   WHEN OTHERS THEN "
        "       IF SQLCODE != -955 THEN "
        "           RAISE; "
        "       END IF; "
        "END;";

    string create_transactions = 
        "BEGIN "
        "   EXECUTE IMMEDIATE 'CREATE TABLE Transactions ("
        "       transaction_id INT PRIMARY KEY, "
        "       user_id INT, "
        "       book_id INT, "
        "       issue_date DATE, "
        "       return_date DATE, "
        "       status VARCHAR2(50), "
        "       FOREIGN KEY (user_id) REFERENCES Users(user_id), "
        "       FOREIGN KEY (book_id) REFERENCES Books(book_id))'; "
        "EXCEPTION "
        "   WHEN OTHERS THEN "
        "       IF SQLCODE != -955 THEN "
        "           RAISE; "
        "       END IF; "
        "END;";

    try {
        this->executeSQL(create_books);  // Use 'this' to refer to the current instance of the class
        cout << "Books table created or already exists." << endl;

        this->executeSQL(create_users); 
        cout << "Users table created or already exists." << endl;

        this->executeSQL(create_transactions); 
        cout << "Transactions table created or already exists." << endl;
    } catch (const exception& e) {
        cerr << "Error creating tables: " << e.what() << endl;
    }
}
};

// Placeholder functions for menu options
void addBook(Database& db) {
    int book_id, available_copies;
    string title, author;

    cout << "Enter Book ID: ";
    cin >> book_id;
    cin.ignore();  // To discard the newline character from the input buffer
    cout << "Enter Book Title: ";
    getline(cin, title);
    cout << "Enter Author Name: ";
    getline(cin, author);
    cout << "Enter Available Copies: ";
    cin >> available_copies;

    string sql = "INSERT INTO Books (book_id, title, author, available_copies) VALUES ("
                 + to_string(book_id) + ", '" + title + "', '" + author + "', " + to_string(available_copies) + ")";

    db.executeSQL(sql);
}

void updateBook(Database& db) {
    int book_id, available_copies;
    string title, author;

    cout << "Enter Book ID to Update: ";
    cin >> book_id;
    cin.ignore();  // To discard the newline character
    cout << "Enter New Title (Leave empty to keep the current): ";
    getline(cin, title);
    cout << "Enter New Author (Leave empty to keep the current): ";
    getline(cin, author);
    cout << "Enter New Available Copies (Enter -1 to keep the current): ";
    cin >> available_copies;

    string sql = "UPDATE Books SET ";

    if (!title.empty()) {
        sql += "title = '" + title + "', ";
    }
    if (!author.empty()) {
        sql += "author = '" + author + "', ";
    }
    if (available_copies != -1) {
        sql += "available_copies = " + to_string(available_copies) + " ";
    }

    // Remove trailing comma and space if present
    if (sql[sql.length() - 2] == ',') {
        sql = sql.substr(0, sql.length() - 2);
    }

    sql += " WHERE book_id = " + to_string(book_id);

    db.executeSQL(sql);
}

void deleteBook(Database& db) {
    int book_id;
    cout << "Enter Book ID to Delete: ";
    cin >> book_id;

    string sql = "DELETE FROM Books WHERE book_id = " + to_string(book_id);

    db.executeSQL(sql);
}

void addUser(Database& db) {
    int user_id;
    string name, contact_info;

    cout << "Enter User ID: ";
    cin >> user_id;
    cin.ignore();  // To discard the newline character
    cout << "Enter User Name: ";
    getline(cin, name);
    cout << "Enter Contact Info: ";
    getline(cin, contact_info);

    string sql = "INSERT INTO Users (user_id, name, contact_info) VALUES ("+ to_string(user_id) + ", '" + name + "', '" + contact_info + "')";

    db.executeSQL(sql);
}

void deleteUser(Database& db) {
    int user_id;
    cout << "Enter User ID to Delete: ";
    cin >> user_id;

    string sql = "DELETE FROM Users WHERE user_id = " + to_string(user_id);

    db.executeSQL(sql);
}

void issueBook(Database& db) {
    int transaction_id, user_id, book_id;
    string issue_date, return_date = "NULL";  // Default return date to NULL
    string status = "Issued";

    cout << "Enter Transaction ID: ";
    cin >> transaction_id;
    cout << "Enter User ID: ";
    cin >> user_id;
    cout << "Enter Book ID: ";
    cin >> book_id;
    cout << "Enter Issue Date (YYYY-MM-DD): ";
    cin >> issue_date;

    // Construct the SQL statement directly
    string sql = "INSERT INTO Transactions (transaction_id, user_id, book_id, issue_date, return_date, status) "
                 "VALUES (" + to_string(transaction_id) + ", " + to_string(user_id) + ", " + to_string(book_id) + ", "
                 "TO_DATE('" + issue_date + "', 'YYYY-MM-DD'), " + return_date + ", '" + status + "')";

    try {
        db.executeSQL(sql);  // Execute the constructed SQL statement
        cout << "Book issued successfully." << endl;
    } catch (const exception& e) {
        cerr << "Error issuing book: " << e.what() << endl;
    }
}

void returnBook(Database& db) {
    int transaction_id;
    string return_date, status = "Returned";

    cout << "Enter Transaction ID: ";
    cin >> transaction_id;
    cout << "Enter Return Date (YYYY-MM-DD): ";
    cin >> return_date;

    string sql = "UPDATE Transactions SET return_date = TO_DATE('" + return_date + "', 'YYYY-MM-DD'), status = '" + status + "' WHERE transaction_id = " + to_string(transaction_id);

    db.executeSQL(sql);
}

void displayBooks(Database& db) {
    string sql = "SELECT book_id, title, author, available_copies FROM Books";
    OCIStmt* stmt = nullptr;
    OCIResult* result = nullptr;
    OCIDefine* def1 = nullptr;
    OCIDefine* def2 = nullptr;
    OCIDefine* def3 = nullptr;
    OCIDefine* def4 = nullptr;

    try {
        checkOCIStatus(OCIHandleAlloc(db.env, (void**)&stmt, OCI_HTYPE_STMT, 0, nullptr), db.err);
        checkOCIStatus(OCIStmtPrepare(stmt, db.err, (const OraText*)sql.c_str(), sql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT), db.err);

        // Execute the SQL query
        checkOCIStatus(OCIStmtExecute(db.svc, stmt, db.err, 0, 0, nullptr, nullptr, OCI_DEFAULT), db.err);

        // Bind the output variables
        int book_id;
        char title[256], author[256];
        int available_copies;

        checkOCIStatus(OCIDefineByPos(stmt, &def1, db.err, 1, &book_id, sizeof(book_id), SQLT_INT, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def2, db.err, 2, title, sizeof(title), SQLT_STR, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def3, db.err, 3, author, sizeof(author), SQLT_STR, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def4, db.err, 4, &available_copies, sizeof(available_copies), SQLT_INT, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);

        // Fetch and display the results
        cout << "Books in the Library:\n";
        cout << left << setw(10) << "Book ID" << setw(30) << "Title" << setw(30) << "Author" << setw(20) << "Available Copies" << endl;
        while (OCIStmtFetch(stmt, db.err, 1, OCI_FETCH_NEXT, OCI_DEFAULT) == OCI_SUCCESS) {
            cout << left << setw(10) << book_id 
                 << setw(30) << title 
                 << setw(30) << author 
                 << setw(20) << available_copies << endl;
        }

        // Clean up
        OCIHandleFree(stmt, OCI_HTYPE_STMT);

    } catch (const exception& e) {
        cerr << "Error displaying books: " << e.what() << endl;
    }
}

void displayTransactions(Database& db) {
    string sql = "SELECT transaction_id, user_id, book_id, issue_date, return_date, status FROM Transactions";
    OCIStmt* stmt = nullptr;
    OCIDefine* def1 = nullptr;
    OCIDefine* def2 = nullptr;
    OCIDefine* def3 = nullptr;
    OCIDefine* def4 = nullptr;
    OCIDefine* def5 = nullptr;
    OCIDefine* def6 = nullptr;

    try {
        checkOCIStatus(OCIHandleAlloc(db.env, (void**)&stmt, OCI_HTYPE_STMT, 0, nullptr), db.err);
        checkOCIStatus(OCIStmtPrepare(stmt, db.err, (const OraText*)sql.c_str(), sql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT), db.err);

        // Execute the SQL query
        checkOCIStatus(OCIStmtExecute(db.svc, stmt, db.err, 0, 0, nullptr, nullptr, OCI_DEFAULT), db.err);

        // Bind the output variables
        int transaction_id, user_id, book_id;
        char issue_date[20], return_date[20], status[50];

        checkOCIStatus(OCIDefineByPos(stmt, &def1, db.err, 1, &transaction_id, sizeof(transaction_id), SQLT_INT, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def2, db.err, 2, &user_id, sizeof(user_id), SQLT_INT, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def3, db.err, 3, &book_id, sizeof(book_id), SQLT_INT, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def4, db.err, 4, issue_date, sizeof(issue_date), SQLT_STR, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def5, db.err, 5, return_date, sizeof(return_date), SQLT_STR, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);
        checkOCIStatus(OCIDefineByPos(stmt, &def6, db.err, 6, status, sizeof(status), SQLT_STR, nullptr, nullptr, nullptr, OCI_DEFAULT), db.err);

        // Fetch and display the results
        cout << "Transactions in the Library:\n";
        cout << left << setw(15) << "Transaction ID" << setw(10) << "User ID" << setw(10) << "Book ID" 
             << setw(15) << "Issue Date" << setw(15) << "Return Date" << setw(30) << "Status" << endl;

        while (OCIStmtFetch(stmt, db.err, 1, OCI_FETCH_NEXT, OCI_DEFAULT) == OCI_SUCCESS) {
            cout << left << setw(15) << transaction_id 
                 << setw(10) << user_id 
                 << setw(10) << book_id 
                 << setw(15) << issue_date 
                 << setw(15) << return_date 
                 << setw(30) << status << endl;
        }

        // Clean up
        OCIHandleFree(stmt, OCI_HTYPE_STMT);

    } catch (const exception& e) {
        cerr << "Error displaying transactions: " << e.what() << endl;
    }
}

int main() {
    Database db;
    try {
        db.connect();

        // Check and create necessary tables
        db.checkAndCreateTables();  // Ensure you are calling this on the db instance.

        int choice;
        do {
            cout << "\nLibrary Management System Menu:\n";
            cout << "1. Add Book\n";
            cout << "2. Update Book\n";
            cout << "3. Delete Book\n";
            cout << "4. Add User\n";
            cout << "5. Delete User\n";
            cout << "6. Issue Book\n";
            cout << "7. Return Book\n";
            cout << "8. Display Books\n";
            cout << "9. Display Transactions\n";
            cout << "10. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;

            switch (choice) {
                case 1: addBook(db); break;
                case 2: updateBook(db); break;
                case 3: deleteBook(db); break;
                case 4: addUser(db); break;
                case 5: deleteUser(db); break;
                case 6: issueBook(db); break;
                case 7: returnBook(db); break;
                case 8: displayBooks(db); break;
                case 9: displayTransactions(db); break;
                case 10: cout << "Exiting program. Goodbye!\n"; break;
                default: cout << "Invalid choice. Please try again.\n";
            }
        } while (choice != 10);

        db.disconnect();
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }
}

