#include "diff_utils.h"
#include <QVector>
#include <QMap>
#include <algorithm>

namespace GitScm {

struct VState {
    QVector<int> v;
    int offset;
};

QList<DiffEdit> computeMyersDiff(const QStringList &oldLines, const QStringList &newLines) {
    int N = oldLines.size();
    int M = newLines.size();
    int maxD = N + M;
    
    QVector<QVector<int>> trace;
    QVector<int> v(2 * maxD + 1, 0);
    int offset = maxD;
    
    for (int d = 0; d <= maxD; ++d) {
        trace.append(v);
        for (int k = -d; k <= d; k += 2) {
            int x;
            if (k == -d || (k != d && v[offset + k - 1] < v[offset + k + 1])) {
                x = v[offset + k + 1];
            } else {
                x = v[offset + k - 1] + 1;
            }
            
            int y = x - k;
            while (x < N && y < M && oldLines[x] == newLines[y]) {
                x++;
                y++;
            }
            
            v[offset + k] = x;
            
            if (x >= N && y >= M) {
                goto found;
            }
        }
    }
    
found:
    QList<DiffEdit> edits;
    int x = N;
    int y = M;
    
    for (int d = trace.size() - 1; d >= 0; --d) {
        const QVector<int> &vCurrent = trace[d];
        int k = x - y;
        
        int prevK;
        if (k == -d || (k != d && vCurrent[offset + k - 1] < vCurrent[offset + k + 1])) {
            prevK = k + 1;
        } else {
            prevK = k - 1;
        }
        
        int prevX = vCurrent[offset + prevK];
        int prevY = prevX - prevK;
        
        while (x > prevX && y > prevY) {
            edits.prepend({DiffEdit::Keep, x - 1, y - 1, oldLines[x - 1]});
            x--;
            y--;
        }
        
        if (d > 0) {
            if (x == prevX) {
                edits.prepend({DiffEdit::Insert, -1, y - 1, newLines[y - 1]});
            } else {
                edits.prepend({DiffEdit::Delete, x - 1, -1, oldLines[x - 1]});
            }
        }
        
        x = prevX;
        y = prevY;
    }
    
    return edits;
}

}
