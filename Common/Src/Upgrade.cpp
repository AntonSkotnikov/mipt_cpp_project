#include "Upgrade.hpp"
#include <algorithm>

namespace plague {

bool dependenciesSatisfied(const UpgradeDefinition & upgrade,
                           const std::vector<UpgradeId> & purchasedUpgrades) {
    return std::all_of(
        upgrade.dependencies.begin(),
        upgrade.dependencies.end(),
        [&purchasedUpgrades](const UpgradeId & dependency) {
            return std::find(purchasedUpgrades.begin(), purchasedUpgrades.end(), dependency) != purchasedUpgrades.end();
        }
    );
}

}
