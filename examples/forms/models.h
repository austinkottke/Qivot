#ifndef FORMS_MODELS_H
#define FORMS_MODELS_H

#include <qivot.h>

/// The demo's records. Every rule a form shows is declared here, once: the QML
/// pages only say which field goes where.

// --- Sign up -------------------------------------------------------------------
class Member : public QiModel {
    QI_MODEL
public:
    QiField<QString> email;
    QiField<QString> name;
    QiField<QString> username;
    QiField<QString> password;
    QiField<QString> password_confirm;
    QiField<QDate>   born;
    QiField<QString> plan;
    QiField<QString> team_name;
    QiField<bool>    terms;
};
QI_DECLARE_MODEL(Member, "member",
    QI_FIELD(email,    QiNotNull | QiUnique | QiTrim() | QiLower() | QiEmail()),
    QI_FIELD(name,     QiLabel("Your name") | QiRequired() | QiCollapseSpaces() | QiLength(2, 60)),
    QI_FIELD(username, QiRequired() | QiLower() | QiPattern("^[a-z0-9_]{3,20}$", "3 to 20 of a-z, 0-9 and _")
                       | QiUniqueIn() | QiNotOneOf({ "admin", "root", "support" }).message("is reserved")),
    QI_FIELD(password, QiRequired() | QiStrongPassword(8)),
    QI_FIELD(password_confirm, QiLabel("Confirm password") | QiRequired() | QiSameAs("password")),
    QI_FIELD(born,     QiLabel("Date of birth") | QiRequired() | QiPast() | QiMinAge(16).message("you must be 16 or over")
                       | QiMaxAge(120).warning().message("is that right?")),
    QI_FIELD(plan,     QiRequired() | QiOneOf({ "Free", "Pro", "Team" })),
    QI_FIELD(team_name, QiRequiredIf("plan", "Team") | QiLength(0, 40)),
    QI_FIELD(terms,    QiLabel("I accept the terms")
                       | QiCustom("accepted", "must be accepted", [](const QVariant &v) { return v.toBool(); }).checkingNull()));

// --- Checkout ------------------------------------------------------------------
static bool isHoliday(const QDate &d) {
    return (d.month() == 12 && (d.day() == 25 || d.day() == 26)) || (d.month() == 1 && d.day() == 1);
}

class Purchase : public QiModel {
    QI_MODEL
public:
    QiField<QString> email;
    QiField<QString> ship_name;
    QiField<QString> ship_address;
    QiField<QString> ship_postcode;
    QiField<QString> ship_country;
    QiField<bool>    billing_same;
    QiField<QString> bill_address;
    QiField<QString> bill_postcode;
    QiField<QString> card_number;
    QiField<QString> card_expiry;
    QiField<QString> cvc;
    QiField<QDate>   deliver_on;
    QiField<QString> promo;
};
QI_DECLARE_MODEL(Purchase, "purchase",
    QI_FIELD(email,        QiRequired() | QiTrim() | QiLower() | QiEmail()),
    QI_FIELD(ship_name,    QiLabel("Name") | QiRequired() | QiCollapseSpaces() | QiLength(2, 60)),
    QI_FIELD(ship_address, QiLabel("Address") | QiRequired() | QiLength(5, 120)),
    QI_FIELD(ship_postcode, QiLabel("Postcode") | QiRequired() | QiUpper() | QiPattern("^[A-Z0-9][A-Z0-9 -]{1,8}[A-Z0-9]$", "is not a postcode")),
    QI_FIELD(ship_country, QiLabel("Country") | QiRequired() | QiOneOf({ "United Kingdom", "United States", "France", "Germany", "Japan" })),
    QI_FIELD(billing_same, QiLabel("Billing address is the same")),
    QI_FIELD(bill_address, QiLabel("Billing address") | QiRequiredIf("billing_same", false) | QiProhibitedIf("billing_same", true)),
    QI_FIELD(bill_postcode, QiLabel("Billing postcode") | QiRequiredIf("billing_same", false) | QiUpper()),
    QI_FIELD(card_number,  QiLabel("Card number") | QiRequired() | QiLuhn()),
    QI_FIELD(card_expiry,  QiLabel("Expiry (MM/YY)") | QiRequired() | QiNotExpired()),
    QI_FIELD(cvc,          QiLabel("CVC") | QiRequired() | QiPattern("^[0-9]{3,4}$", "is 3 or 4 digits")),
    QI_FIELD(deliver_on,   QiLabel("Delivery date") | QiRequired() | QiDateMin("today+2d").message("is too soon: we need 2 days")
                           | QiDateMax("today+60d") | QiBusinessDay(isHoliday).message("we deliver on working days")),
    QI_FIELD(promo,        QiLabel("Promo code") | QiUpper()
                           | QiOneOf({ "WELCOME10", "SPRING", "FRIENDS" }).warning().message("isn't a code we know: it'll be ignored")));

class OrderLine : public QiModel {
    QI_MODEL
public:
    QiField<QString> product;
    QiField<int>     quantity;
    QiField<double>  unit_price;
};
QI_DECLARE_MODEL(OrderLine, "order_line",
    QI_FIELD(product,    QiRequired()),
    QI_FIELD(quantity,   QiRequired() | QiPositive().message("at least 1") | QiMax(20).message("20 at most per order")
                         | QiMax(10).warning().message("that's a lot: are you sure?")),
    QI_FIELD(unit_price, QiPositive() | QiDecimals(2)));

// --- Booking -------------------------------------------------------------------
class Room : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<int>     seats;
};
QI_DECLARE_MODEL(Room, "room", QI_FIELD(name, QiRequired()), QI_FIELD(seats, QiPositive()));

class Booking : public QiModel {
    QI_MODEL
public:
    QiField<QString>   room;
    QiField<QString>   organiser;
    QiField<QDateTime> starts_at;
    QiField<QDateTime> ends_at;
    QiField<int>       people;
};
QI_DECLARE_MODEL(Booking, "booking",
    QI_FIELD(room,      QiRequired() | QiExists("room", "name")),
    QI_FIELD(organiser, QiRequired() | QiCollapseSpaces() | QiLength(2, 60)),
    QI_FIELD(starts_at, QiLabel("Starts") | QiRequired() | QiFuture().message("must be in the future") | QiWeekday()
                        | QiTimeBetween(QTime(8, 0), QTime(19, 30)).message("we're open 08:00 to 20:00")
                        | QiTimeStep(15).message("must be on the quarter hour")),
    QI_FIELD(ends_at,   QiLabel("Ends") | QiRequired() | QiAfter("starts_at")
                        | QiMinutesAfter("starts_at", 30, 240).message("a booking is 30 minutes to 4 hours")
                        | QiTimeBetween(QTime(8, 30), QTime(20, 0)).message("we close at 20:00")
                        | QiTimeStep(15).message("must be on the quarter hour")
                        | QiNoOverlap("starts_at", { "room" }).message("overlaps another booking of this room")),
    QI_FIELD(people,    QiRequired() | QiRange(1, 40) | QiMax(12).warning().message("more than the small rooms seat")));

// --- Wizard --------------------------------------------------------------------
class Applicant : public QiModel {
    QI_MODEL
public:
    QiField<QString> first_name;
    QiField<QString> last_name;
    QiField<QString> email;
    QiField<QDate>   born;
    QiField<QString> country;
    QiField<QString> state;
    QiField<QString> postcode;
    QiField<QString> phone;
    QiField<bool>    employed;
    QiField<QString> employer;
    QiField<QDate>   started;
    QiField<double>  income;
};
QI_DECLARE_MODEL(Applicant, "applicant",
    QI_FIELD(first_name, QiRequired() | QiCollapseSpaces() | QiLength(1, 40)),
    QI_FIELD(last_name,  QiRequired() | QiCollapseSpaces() | QiLength(1, 40)),
    QI_FIELD(email,      QiRequired() | QiTrim() | QiLower() | QiEmail()),
    QI_FIELD(born,       QiLabel("Date of birth") | QiRequired() | QiPast() | QiMinAge(18).message("you must be 18 or over to apply")),
    QI_FIELD(country,    QiRequired() | QiOneOf({ "United States", "Canada", "United Kingdom", "Australia" })),
    QI_FIELD(state,      QiLabel("State") | QiRequiredIf("country", "United States")
                         | QiProhibitedIf("country", "United Kingdom").message("isn't used in the UK")),
    QI_FIELD(postcode,   QiLabel("ZIP / postcode") | QiRequired() | QiUpper()),
    QI_FIELD(phone,      QiRequired() | QiPhone()),
    QI_FIELD(employed,   QiLabel("I'm employed")),
    QI_FIELD(employer,   QiRequiredIf("employed", true)),
    QI_FIELD(started,    QiLabel("Started there on") | QiRequiredIf("employed", true) | QiTodayOrEarlier()
                         | QiAfter("born").message("can't be before you were born")),
    QI_FIELD(income,     QiLabel("Yearly income") | QiNonNegative() | QiDecimals(2)));

// --- Inventory -----------------------------------------------------------------
class Product : public QiModel {
    QI_MODEL
public:
    QiField<QString> sku;
    QiField<QString> name;
    QiField<double>  price;
    QiField<int>     stock;
    QiField<QDate>   launched;
};
QI_DECLARE_MODEL(Product, "product",
    QI_FIELD(sku,      QiLabel("SKU") | QiRequired() | QiTrim() | QiUpper() | QiPattern("^[A-Z0-9-]{3,12}$", "3 to 12 of A-Z, 0-9 and -")
                       | QiUniqueIn()),
    QI_FIELD(name,     QiRequired() | QiLength(2, 40)),
    QI_FIELD(price,    QiRequired() | QiPositive() | QiDecimals(2) | QiMax(5000).warning().message("is unusually high")),
    QI_FIELD(stock,    QiRequired() | QiNonNegative()),
    QI_FIELD(launched, QiDateMin("2015-01-01") | QiTodayOrEarlier()));

#endif // FORMS_MODELS_H
