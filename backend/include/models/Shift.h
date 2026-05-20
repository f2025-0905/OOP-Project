#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================
// ABSTRACT BASE CLASS: ShiftEntry
// Demonstrates OOP: Abstraction + Polymorphism
// ============================================
class ShiftEntry {
protected:
    std::string id;
    std::string employeeId;
    std::string positionId;
    std::string shiftDate;
    std::string startTime;
    std::string endTime;
    std::string notes;
    std::string createdBy;
    std::string createdAt;

public:
    ShiftEntry() = default;
    ShiftEntry(const std::string& id, const std::string& employeeId,
               const std::string& positionId, const std::string& shiftDate,
               const std::string& startTime, const std::string& endTime);

    virtual ~ShiftEntry() = default;

    // Getters
    std::string getId() const { return id; }
    std::string getEmployeeId() const { return employeeId; }
    std::string getPositionId() const { return positionId; }
    std::string getShiftDate() const { return shiftDate; }
    std::string getStartTime() const { return startTime; }
    std::string getEndTime() const { return endTime; }
    std::string getNotes() const { return notes; }

    // Setters
    void setNotes(const std::string& n) { notes = n; }
    void setCreatedBy(const std::string& cb) { createdBy = cb; }

    double getDurationHours() const;

    // Pure virtual — polymorphic
    virtual std::string getStatus() const = 0;
    virtual bool canEdit() const = 0;
    virtual json toJson() const = 0;
};

// ============================================
// DERIVED: DraftShift
// ============================================
class DraftShift : public ShiftEntry {
public:
    using ShiftEntry::ShiftEntry;

    std::string getStatus() const override { return "draft"; }
    bool canEdit() const override { return true; }
    json toJson() const override;
};

// ============================================
// DERIVED: PublishedShift
// ============================================
class PublishedShift : public ShiftEntry {
private:
    std::string publishedAt;

public:
    using ShiftEntry::ShiftEntry;

    void setPublishedAt(const std::string& t) { publishedAt = t; }
    std::string getPublishedAt() const { return publishedAt; }

    std::string getStatus() const override { return "published"; }
    bool canEdit() const override { return false; }
    json toJson() const override;
};

// ============================================
// ABSTRACT BASE CLASS: Request
// ============================================
class Request {
protected:
    std::string id;
    std::string requesterId;
    std::string status;
    std::string createdAt;

public:
    Request() = default;
    virtual ~Request() = default;

    std::string getId() const { return id; }
    std::string getStatus() const { return status; }
    void setStatus(const std::string& s) { status = s; }

    virtual std::string getType() const = 0;
    virtual bool approve() = 0;
    virtual bool reject() = 0;
    virtual json toJson() const = 0;
};

// ============================================
// DERIVED: LeaveRequest
// ============================================
class LeaveRequest : public Request {
private:
    std::string startDate;
    std::string endDate;
    std::string leaveType;
    std::string reason;
    std::string reviewedBy;
    std::string reviewedAt;
    int totalDays;

public:
    LeaveRequest() = default;

    void setId(const std::string& i) { id = i; }
    void setRequesterId(const std::string& r) { requesterId = r; }
    void setStartDate(const std::string& d) { startDate = d; }
    void setEndDate(const std::string& d) { endDate = d; }
    void setLeaveType(const std::string& t) { leaveType = t; }
    void setReason(const std::string& r) { reason = r; }
    void setReviewedBy(const std::string& rb) { reviewedBy = rb; }
    void setTotalDays(int d) { totalDays = d; }
    void setCreatedAt(const std::string& c) { createdAt = c; }

    std::string getRequesterId() const { return requesterId; }
    std::string getStartDate() const { return startDate; }
    std::string getEndDate() const { return endDate; }
    std::string getLeaveType() const { return leaveType; }
    std::string getReason() const { return reason; }
    int getTotalDays() const { return totalDays; }

    std::string getType() const override { return "leave"; }
    bool approve() override;
    bool reject() override;
    json toJson() const override;
};
