#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QStringList>
#include <QVariantList>
#include <vector>

namespace hamclock {

class AppSettings;

// DE -> DX HF reliability by band for the next 24 h, from the in-house path model
// (bands/path_model.hpp; approximate, not VOACAP). Inputs: NOAA SWPC SFI and Kp.
class BandsMatrixModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasMatrix READ hasMatrix NOTIFY matrixChanged)
    Q_PROPERTY(bool hasPath READ hasPath NOTIFY matrixChanged)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY matrixChanged)  // {band, now, muf, cells[24]} 0..1
    Q_PROPERTY(QString pathText READ pathText NOTIFY matrixChanged)
    Q_PROPERTY(QString bestText READ bestText NOTIFY matrixChanged)
    Q_PROPERTY(QString startText READ startText NOTIFY matrixChanged)
public:
    explicit BandsMatrixModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    // False until SFI and Kp are known and DE and DX are different places.
    bool hasMatrix() const { return has_matrix_; }
    bool hasPath() const { return has_path_; }
    QVariantList rows() const;
    QString pathText() const { return path_text_; }  // "4880 km · SSB 100 W"
    QString bestText() const { return best_text_; }  // "Best now: 20m 87%"
    QString startText() const { return start_text_; } // "00Z" first column hour (UTC)
    PolledSource& kpSource() { return *kp_source_; }
    PolledSource& fluxSource() { return *flux_source_; }

signals:
    void matrixChanged();

private:
    void parse();
    void recompute();
    void refreshState();

    AppSettings* settings_;
    PolledSource* kp_source_;
    PolledSource* flux_source_;
    double sfi_ = 0, kp_ = 0;
    bool have_inputs_ = false, has_matrix_ = false, has_path_ = false;
    qint64 start_hour_ = -1;  // UTC hour the matrix was computed for
    QStringList bands_;
    std::vector<double> muf_;
    std::vector<std::vector<double>> cells_;
    QString path_text_, best_text_, start_text_;
};

void register_widget_bandsmatrix(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
