/**
    @author Ben Lau
 */

#ifndef QiSTREAM_H
#define QiSTREAM_H

#include <qimodel.h>

/// QiStream class provides a stream interface for reading and writing data model field
/**

    Using QiStrem's streaming operator , you could read and write the
    data model field in a simplifer way.

    Assume you have the following data model:

\code

/// An example model for people's height and weight data
class HealthCheck : public QiModel {
    QI_MODEL
public:

    QiField<QString> name;
    QiField<int>     height;
    QiField<double>  weight;
};

QI_DECLARE_MODEL(HealthCheck,
                 "healthcheck",
                 QI_FIELD(name , QiNotNull),
                 QI_FIELD(height),
                 QI_FIELD(weight)
                 );
\endcode

    You can read and write using the streaming operators

    Write example:
\code
    HealthCheck record;
    QiStream stream(&record);

    stream << "Tester 1" << 179 << 120.5;
    //Then record = ["Tester 1" , 179,120.5]
\endcode

    Read example:

\code
    QiStream stream(&record);
    QString name;
    QString height;
    QString weight;

    stream >> name >> height >> weight;
\endcode

    @remarks The sequence of read/write is equal to the registration order in QI_DECLARE_MODEL. It is nothing related to its declaration order in class declaration.
 */
class QiStream
{
public:
    /// Default constructor
    QiStream();

    /// Construct a QiStream operates on target model
    /**
      The constructor will open the target model for read/write automatically
      @see open
     */
    QiStream(QiAbstractModel* model);

    /// Open and set the target model for read / write
    /**
      @param model The target model for read / write. The model must contains more than a field. Otherwise the operation will be failed.
      @remarks It will reset the value returned by currentField()
     */
    void open(QiAbstractModel* model);

    /// Get the target model
    QiAbstractModel* model();

    /// Get the index of the current reading/ writing field
    int currentField();

    /// Write the value to data model at current field
    /**
      @remarks The "id" field is ignored in writing
     */
    void write(const QVariant value);

    /// Read the current field and save to target variable
    void read(QVariant &target);

    /// Close the stream and release the resource holding
    void close();

    /// Write the value to data model at current field
    /**
      @remarks It is a wrapper of write().
     */
    QiStream& operator<< (const QVariant value);

    /// Read the current field and save to target variable
    /**
      @remarks It is a wrapper of read().
     */
    QiStream& operator>> (QVariant &target);

    /// Read the current field and save to target variable
    /**
      @remarks It is a wrapper of read().
     */
    QiStream& operator>> (QString& target);

    /// Read the current field and save to target variable
    /**
      @remarks It is a wrapper of read().
     */
    QiStream& operator>> (int& target);

    /// Read the current field and save to target variable
    /**
      @remarks It is a wrapper of read().
     */
    QiStream& operator>> (double& target);

    /// Read the current field and save to target variable
    /**
      @remarks It is a wrapper of read().
     */
    QiStream& operator>> (QDateTime& target);

private:
    QiAbstractModel* m_model;

    /// The current read/write field
    int m_current;
};

#endif // QiSTREAM_H
