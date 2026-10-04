# Forms — validation declared once, shown live in QML

Every rule in this demo is in [`models.h`](models.h), on the fields:

```c++
QI_FIELD(email, QiNotNull | QiUnique | QiTrim() | QiLower() | QiEmail()),
QI_FIELD(born,  QiRequired() | QiPast() | QiMinAge(16).message("you must be 16 or over")),
```

The QML pages only say which field goes where:

```qml
QiForm      { id: signup; model: "Member" }
QiTextField { form: signup; field: "email" }
QiDateField { form: signup; field: "born" }
QiSubmitButton { form: signup; text: "Create account" }
```

> **Run it**
> ```sh
> cd examples/forms
> qmake && make
> ./forms
> ```
> `QIVOT_SELFTEST=1 QT_QPA_PLATFORM=offscreen ./forms` drives every page headlessly
> and fails on a wrong answer or a QML warning.

## The pages

| Page | What it shows |
|---|---|
| **Sign up** | Errors once you leave a field; the email trimmed, lower-cased and checked against existing accounts (`taken@example.com` is taken); a strong password and its confirmation (`QiSameAs`); 16 or over (`QiMinAge`), and a *warning* past 120; a team name only for the Team plan (`QiRequiredIf`); terms that must be accepted; one field wired with the attached `QiForm.field` instead of a ready-made control. |
| **Checkout** | Each basket line is its own form, checked with the order. The billing address only when it differs (`QiRequiredIf` / `QiProhibitedIf`); card numbers (`QiLuhn`) and expiry (`QiNotExpired`, valid through the month); delivery 2 to 60 days out (`QiDateMin("today+2d")`) on a working day that isn't a holiday (`QiBusinessDay`); an unknown promo code as a warning. |
| **Booking** | Rooms (`QiExists`); weekdays (`QiWeekday`), opening hours (`QiTimeBetween`), quarter hours (`QiTimeStep(15)`), 30 minutes to 4 hours (`QiMinutesAfter`), and no overlap with that room's bookings (`QiNoOverlap`). "abc" people is "must be a whole number"; more than 12 is a warning. |
| **Wizard** | Three steps; Next checks only that step (`validateFields`). A US address needs a state, the UK doesn't use one; employment details only when employed (`QiRequiredIf("employed", true)`), not before your birth (`QiAfter("born")`). |
| **Inventory** | An editable table: a `QiForm` per row, loaded by id, errors under each cell, unique upper-cased SKUs (`QiUniqueIn`), a warning on prices over 5,000, saved row by row. |

## Files

| File | Role |
|---|---|
| `models.h` | The records and every rule. |
| `main.cpp` | Opens a fresh SQLite file, seeds it, registers QML (`qiRegisterQml`), and the self-test. |
| `*Page.qml` | One page each; `DemoPage.qml` is the scrolling layout they share. |
