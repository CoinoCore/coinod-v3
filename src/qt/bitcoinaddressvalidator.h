#ifndef BITCOINADDRESSVALIDATOR_H
#define BITCOINADDRESSVALIDATOR_H

#include <QRegularExpressionValidator>

/** Base48 entry widget validator.
   Corrects near-miss characters and refuses characters that are no part of base48.
 */
class BitcoinAddressValidator : public QRegularExpressionValidator
{
    Q_OBJECT
public:
    explicit BitcoinAddressValidator(QObject *parent = 0);

    QRegularExpressionValidator::State validate(QString &input, int &pos) const;

    static const int MaxAddressLength = 35;
signals:

public slots:

};

#endif // BITCOINADDRESSVALIDATOR_H
