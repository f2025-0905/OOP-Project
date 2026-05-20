#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================
// BASE CLASS: User
// Demonstrates OOP: Encapsulation + Abstraction
// ============================================
class User {
protected:
    std::string id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string firstName;
    std::string lastName;
    std::string role;
    bool isActive;
    std::string createdAt;

public:
    User() = default;
    User(const std::string& id, const std::string& username,
         const std::string& email, const std::string& firstName,
         const std::string& lastName, const std::string& role);

    virtual ~User() = default;

    // Getters
    std::string getId() const { return id; }
    std::string getUsername() const { return username; }
    std::string getEmail() const { return email; }
    std::string getFirstName() const { return firstName; }
    std::string getLastName() const { return lastName; }
    std::string getRole() const { return role; }
    std::string getFullName() const { return firstName + " " + lastName; }
    bool getIsActive() const { return isActive; }

    // Setters
    void setPasswordHash(const std::string& hash) { passwordHash = hash; }
    void setIsActive(bool active) { isActive = active; }
    void setEmail(const std::string& e) { email = e; }

    // Pure virtual: forces subclasses to implement
    virtual json toJson() const = 0;
    virtual std::string getPermissionLevel() const = 0;

    // Static factory from DB row
    static std::shared_ptr<User> fromRow(const pqxx::row& row);
};

// ============================================
// DERIVED CLASS: Employee
// Demonstrates OOP: Inheritance + Polymorphism
// ============================================
class Employee : public User {
private:
    std::string positionId;
    std::string positionName;
    int leaveBalance;
    std::string phone;
    std::string hireDate;

public:
    Employee() = default;
    Employee(const std::string& id, const std::string& username,
             const std::string& email, const std::string& firstName,
             const std::string& lastName);

    std::string getPositionId() const { return positionId; }
    std::string getPositionName() const { return positionName; }
    int getLeaveBalance() const { return leaveBalance; }

    void setPositionId(const std::string& pid) { positionId = pid; }
    void setPositionName(const std::string& pname) { positionName = pname; }
    void setLeaveBalance(int balance) { leaveBalance = balance; }
    void setPhone(const std::string& p) { phone = p; }
    void setHireDate(const std::string& d) { hireDate = d; }

    bool deductLeave(int days);
    void restoreLeave(int days) { leaveBalance += days; }

    // Polymorphic override
    json toJson() const override;
    std::string getPermissionLevel() const override { return "standard"; }
};

// ============================================
// DERIVED CLASS: Manager
// Demonstrates OOP: Inheritance + Polymorphism
// ============================================
class Manager : public User {
private:
    std::string department;

public:
    Manager() = default;
    Manager(const std::string& id, const std::string& username,
            const std::string& email, const std::string& firstName,
            const std::string& lastName);

    std::string getDepartment() const { return department; }
    void setDepartment(const std::string& dept) { department = dept; }

    // Polymorphic override
    json toJson() const override;
    std::string getPermissionLevel() const override { return "admin"; }
};
