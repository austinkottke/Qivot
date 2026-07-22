#ifndef QiCOLUMNS_H
#define QiCOLUMNS_H

/** @file qicolumns.h
    @brief Preprocessor helpers behind the automatic Model::col() descriptor.

    QI_DECLARE_MODEL consumes each QI_FIELD(...) entry twice: once to build the
    runtime metadata array, and once to generate a typed column descriptor so
    you can name columns through the class instead of a hand-typed string:

\code
    auto U = User::col();
    auto top = User::objects()
                   .filter( U.karma > 100 && U.userId != "admin" )
                   .all();

    User u;
    u.load( User::col().userId == "anonymous" );
\endcode

    Because each `.field` is a real member (plus the built-in `.id`), renaming a
    column turns a stale query into a compile error at the use site. col() yields
    a QiWhere, exactly like qiField(&User::field) (see qifieldref.h), so the two
    styles mix freely.

    This is wired in automatically for every model — there is nothing extra to
    write. These macros are internal; do not use them directly.
 */

// --- small preprocessor FOR_EACH (supports up to 50 columns) ---------------
#define QI_COL_EXPAND(x) x

#define QI_COL_FE_1(m, x)       m(x)
#define QI_COL_FE_2(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_1(m, __VA_ARGS__))
#define QI_COL_FE_3(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_2(m, __VA_ARGS__))
#define QI_COL_FE_4(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_3(m, __VA_ARGS__))
#define QI_COL_FE_5(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_4(m, __VA_ARGS__))
#define QI_COL_FE_6(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_5(m, __VA_ARGS__))
#define QI_COL_FE_7(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_6(m, __VA_ARGS__))
#define QI_COL_FE_8(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_7(m, __VA_ARGS__))
#define QI_COL_FE_9(m, x, ...)  m(x) QI_COL_EXPAND(QI_COL_FE_8(m, __VA_ARGS__))
#define QI_COL_FE_10(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_9(m, __VA_ARGS__))
#define QI_COL_FE_11(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_10(m, __VA_ARGS__))
#define QI_COL_FE_12(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_11(m, __VA_ARGS__))
#define QI_COL_FE_13(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_12(m, __VA_ARGS__))
#define QI_COL_FE_14(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_13(m, __VA_ARGS__))
#define QI_COL_FE_15(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_14(m, __VA_ARGS__))
#define QI_COL_FE_16(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_15(m, __VA_ARGS__))
#define QI_COL_FE_17(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_16(m, __VA_ARGS__))
#define QI_COL_FE_18(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_17(m, __VA_ARGS__))
#define QI_COL_FE_19(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_18(m, __VA_ARGS__))
#define QI_COL_FE_20(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_19(m, __VA_ARGS__))
#define QI_COL_FE_21(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_20(m, __VA_ARGS__))
#define QI_COL_FE_22(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_21(m, __VA_ARGS__))
#define QI_COL_FE_23(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_22(m, __VA_ARGS__))
#define QI_COL_FE_24(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_23(m, __VA_ARGS__))
#define QI_COL_FE_25(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_24(m, __VA_ARGS__))
#define QI_COL_FE_26(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_25(m, __VA_ARGS__))
#define QI_COL_FE_27(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_26(m, __VA_ARGS__))
#define QI_COL_FE_28(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_27(m, __VA_ARGS__))
#define QI_COL_FE_29(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_28(m, __VA_ARGS__))
#define QI_COL_FE_30(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_29(m, __VA_ARGS__))
#define QI_COL_FE_31(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_30(m, __VA_ARGS__))
#define QI_COL_FE_32(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_31(m, __VA_ARGS__))
#define QI_COL_FE_33(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_32(m, __VA_ARGS__))
#define QI_COL_FE_34(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_33(m, __VA_ARGS__))
#define QI_COL_FE_35(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_34(m, __VA_ARGS__))
#define QI_COL_FE_36(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_35(m, __VA_ARGS__))
#define QI_COL_FE_37(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_36(m, __VA_ARGS__))
#define QI_COL_FE_38(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_37(m, __VA_ARGS__))
#define QI_COL_FE_39(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_38(m, __VA_ARGS__))
#define QI_COL_FE_40(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_39(m, __VA_ARGS__))
#define QI_COL_FE_41(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_40(m, __VA_ARGS__))
#define QI_COL_FE_42(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_41(m, __VA_ARGS__))
#define QI_COL_FE_43(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_42(m, __VA_ARGS__))
#define QI_COL_FE_44(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_43(m, __VA_ARGS__))
#define QI_COL_FE_45(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_44(m, __VA_ARGS__))
#define QI_COL_FE_46(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_45(m, __VA_ARGS__))
#define QI_COL_FE_47(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_46(m, __VA_ARGS__))
#define QI_COL_FE_48(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_47(m, __VA_ARGS__))
#define QI_COL_FE_49(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_48(m, __VA_ARGS__))
#define QI_COL_FE_50(m, x, ...) m(x) QI_COL_EXPAND(QI_COL_FE_49(m, __VA_ARGS__))

#define QI_COL_FE_NTH( \
    _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17, \
    _18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33, \
    _34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, N, ...) N

/// Apply macro `m` to each argument (each argument may itself be a (…) tuple).
#define QI_COL_FOR_EACH(m, ...) QI_COL_EXPAND(QI_COL_FE_NTH(__VA_ARGS__, \
    QI_COL_FE_50,QI_COL_FE_49,QI_COL_FE_48,QI_COL_FE_47,QI_COL_FE_46, \
    QI_COL_FE_45,QI_COL_FE_44,QI_COL_FE_43,QI_COL_FE_42,QI_COL_FE_41, \
    QI_COL_FE_40,QI_COL_FE_39,QI_COL_FE_38,QI_COL_FE_37,QI_COL_FE_36, \
    QI_COL_FE_35,QI_COL_FE_34,QI_COL_FE_33,QI_COL_FE_32,QI_COL_FE_31, \
    QI_COL_FE_30,QI_COL_FE_29,QI_COL_FE_28,QI_COL_FE_27,QI_COL_FE_26, \
    QI_COL_FE_25,QI_COL_FE_24,QI_COL_FE_23,QI_COL_FE_22,QI_COL_FE_21, \
    QI_COL_FE_20,QI_COL_FE_19,QI_COL_FE_18,QI_COL_FE_17,QI_COL_FE_16, \
    QI_COL_FE_15,QI_COL_FE_14,QI_COL_FE_13,QI_COL_FE_12,QI_COL_FE_11, \
    QI_COL_FE_10,QI_COL_FE_9,QI_COL_FE_8,QI_COL_FE_7,QI_COL_FE_6, \
    QI_COL_FE_5,QI_COL_FE_4,QI_COL_FE_3,QI_COL_FE_2,QI_COL_FE_1)(m, __VA_ARGS__))

// --- consumers of a QI_FIELD(...) tuple, e.g. (userId, QiNotNull|QiUnique) --
//
// Metadata: expands to a QiModelMetaInfoField* (Table / m are in scope inside
// the generated fields() function). A trailing comma separates array elements.
#define QI_FIELD_META(field, ...) \
    new QiModelMetaInfoField(#field, offsetof(Table,field), m.field.type(), m.field.clause() , ##__VA_ARGS__)
#define QI_COL_EMIT_META(tuple) QI_FIELD_META tuple ,

// Column descriptor member: a named QiWhere carrying the column name.
#define QI_FIELD_COLUMN(field, ...) QiWhere field{QStringLiteral(#field)};
#define QI_COL_EMIT_COLUMN(tuple) QI_FIELD_COLUMN tuple

/// Generate the nested Model::Columns struct and the static Model::col().
#define QI_GEN_COLUMNS(MODEL, ...) \
    struct MODEL::Columns { \
        QiWhere id{QStringLiteral("id")}; \
        QI_COL_FOR_EACH(QI_COL_EMIT_COLUMN, __VA_ARGS__) \
    }; \
    inline MODEL::Columns MODEL::col() { return MODEL::Columns(); }

/// As QI_GEN_COLUMNS, but WITHOUT the built-in `.id` member — for a
/// QI_DECLARE_MODEL_NOID model whose table has no `id` column.
#define QI_GEN_COLUMNS_NOID(MODEL, ...) \
    struct MODEL::Columns { \
        QI_COL_FOR_EACH(QI_COL_EMIT_COLUMN, __VA_ARGS__) \
    }; \
    inline MODEL::Columns MODEL::col() { return MODEL::Columns(); }

#endif // QiCOLUMNS_H
