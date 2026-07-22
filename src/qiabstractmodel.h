#ifndef QiABSTRACTMODEL_H
#define QiABSTRACTMODEL_H

#include <QString>

class QiModelMetaInfo;

/// Abstract Data Model
/** It is the based class of QiModel.
  @remarks Never derive your own class based on QiAbstractModel. Due to optimization , many classes like QiListWriter assume that the only derived class is QiModel.
 */
class QiAbstractModel
{
public:
    QiAbstractModel();
    virtual ~QiAbstractModel();

    /// Get the meta info object of the model.
    virtual QiModelMetaInfo *metaInfo() const;

    /// Save the record to database
    /**
      @param forceInsert TRUE if the data should be inserted to the database as a new record regardless of the original id. The id field will be updated after operation.
      @param forceAllField TRUE if all the field should be saved no matter it is null or not. If false, then null field will be skipped.

      If the id is not set , the record will be inserted to the database , then id field will be updated automatically.
      The successive call will update the record instead of insert unless forceInsert is TRUE.

     */
    virtual bool save(bool forceInsert = false , bool forceAllField = false) = 0;

};

#endif // QiABSTRACTMODEL_H
