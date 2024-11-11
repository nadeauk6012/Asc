#include "AuctionatorBidder.h"
#include "Auctionator.h"
#include "ObjectMgr.h"
#include <random>
#include <chrono>

AuctionatorBidder::AuctionatorBidder(uint32 auctionHouseIdParam, ObjectGuid buyer, AuctionatorConfig* auctionatorConfig)
{
    SetLogPrefix("[AuctionatorBidder] ");
    auctionHouseId = auctionHouseIdParam;
    buyerGuid = buyer;
    ahMgr = sAuctionMgr->GetAuctionsMapByHouseId(auctionHouseId);
    config = auctionatorConfig;
    bidOnOwn = config->bidOnOwn;
}

AuctionatorBidder::~AuctionatorBidder()
{

}

void AuctionatorBidder::SpendSomeCash()
{
    uint32 auctionatorPlayerGuid = buyerGuid.GetRawValue();

    std::string query = R"(
        SELECT
            ah.id
        FROM auctionhouse ah
        WHERE itemowner <> {} AND houseid = {};
    )";

    uint32 ownerToSkip = bidOnOwn ? 0 : auctionatorPlayerGuid;

    QueryResult result = CharacterDatabase.Query(query, ownerToSkip, auctionHouseId);

    if (!result) {
        logInfo("Can't see player auctions at ["
            + std::to_string(auctionHouseId) + "] not from ["
            + std::to_string(auctionatorPlayerGuid) + "], moving on.");
        return;
    }

    if (result->GetRowCount() == 0) {
        logInfo("No player auctions, taking my money elsewhere.");
        return;
    }

    std::vector<uint32> biddableAuctionIds;
    biddableAuctionIds.reserve(result->GetRowCount());
    do {
        biddableAuctionIds.push_back(result->Fetch()->Get<uint32>());
    } while (result->NextRow());

    std::shuffle(
        biddableAuctionIds.begin(),
        biddableAuctionIds.end(),
        std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count())
    );

    logInfo("Found " + std::to_string(biddableAuctionIds.size()) + " biddable auctions");

    uint32 purchasePerCycle = GetAuctionsPerCycle();
    uint32 counter = 0;

    while (purchasePerCycle > 0 && !biddableAuctionIds.empty()) {
        counter++;
        AuctionEntry* auction = GetAuctionForPurchase(biddableAuctionIds);

        if (auction == nullptr) {
            return;
        }

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(auction->item_template);

        logInfo("Considering auction: "
            + itemTemplate->Name1
            + " [AuctionId: " + std::to_string(auction->Id) + "]"
            + " [ItemId: " + std::to_string(itemTemplate->ItemId) + "]"
            + " <> " + std::to_string(counter)
        );

        bool success = auction->buyout > 0 ? BuyoutAuction(auction, itemTemplate) : BidOnAuction(auction, itemTemplate);

        if (success) {
            purchasePerCycle--;
            logInfo("Purchase made, remaining: " + std::to_string(purchasePerCycle));
        }
    }
}

AuctionEntry* AuctionatorBidder::GetAuctionForPurchase(std::vector<uint32>& auctionIds)
{
    if (auctionIds.empty()) {
        return nullptr;
    }

    uint32 auctionId = auctionIds.front();
    auctionIds.erase(auctionIds.begin());

    logTrace("Auction removed, remaining items: " + std::to_string(auctionIds.size()));

    return ahMgr->GetAuction(auctionId);
}

bool AuctionatorBidder::BidOnAuction(AuctionEntry* auction, ItemTemplate const* itemTemplate)
{
    if (auction->bid) {
        logInfo("Skipping auction, already bid: "
            + std::to_string(auction->bid) + ".");
        return false;
    }

    uint32 currentPrice = auction->startbid;
    uint32 buyPrice = CalculateBuyPrice(auction, itemTemplate);

    if (currentPrice > buyPrice) {
        logInfo("Skipping auction ("
            + std::to_string(auction->Id) + "), price of "
            + std::to_string(currentPrice) + " is higher than template price of ("
            + std::to_string(buyPrice) + ")"
        );
        return false;
    }

    uint32 bidPrice = currentPrice + (buyPrice - currentPrice) / 2;

    auction->bidder = buyerGuid;
    auction->bid = bidPrice;

    CharacterDatabase.Execute(R"(
        UPDATE
            auctionhouse
        SET
            buyguid = {},
            lastbid = {}
        WHERE
            id = {}
    )",
        auction->bidder.GetCounter(),
        auction->bid,
        auction->Id);

    logInfo("Bid on auction of "
        + itemTemplate->Name1 + " ["
        + std::to_string(auction->Id) + "] of "
        + std::to_string(bidPrice) + " copper."
    );

    return true;
}

bool AuctionatorBidder::BuyoutAuction(AuctionEntry* auction, ItemTemplate const* itemTemplate)
{
    uint32 buyPrice = CalculateBuyPrice(auction, itemTemplate);

    if (auction->buyout > buyPrice) {
        logInfo("Skipping buyout, price ("
            + std::to_string(auction->buyout) + ") is higher than template buyprice ("
            + std::to_string(buyPrice) + ")");
        return false;
    }

    auto trans = CharacterDatabase.BeginTransaction();

    auction->bidder = buyerGuid;
    auction->bid = auction->buyout;

    sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
    auction->DeleteFromDB(trans);

    logInfo("Purchased auction of "
        + itemTemplate->Name1 + " ["
        + std::to_string(auction->Id) + "]"
        + "x" + std::to_string(auction->itemCount) + " for "
        + std::to_string(auction->buyout) + " copper."
    );

    sAuctionMgr->RemoveAItem(auction->item_guid);
    ahMgr->RemoveAuction(auction);

    CharacterDatabase.CommitTransaction(trans);

    return true;
}

uint32 AuctionatorBidder::GetAuctionsPerCycle()
{
    switch (auctionHouseId) {
    case AUCTIONHOUSE_ALLIANCE:
        return config->allianceBidder.maxPerCycle;
    case AUCTIONHOUSE_HORDE:
        return config->hordeBidder.maxPerCycle;
    case AUCTIONHOUSE_NEUTRAL:
        return config->neutralBidder.maxPerCycle;
    default:
        return 0;
    }
}

uint32 AuctionatorBidder::CalculateBuyPrice(AuctionEntry* auction, ItemTemplate const* item)
{
    uint32 marketPrice = 0;
    std::string query = R"(
        SELECT
            average_price
        FROM mod_auctionator_market_price
        WHERE entry = {}
        ORDER BY scan_datetime DESC
        LIMIT 1
    )";
    QueryResult result = CharacterDatabase.Query(query, item->ItemId);

    if (result) {
        marketPrice = result->Fetch()->Get<uint32>();
    }

    uint32 stackSize = item->GetMaxStackSize() > 1 && auction->itemCount > 1 ? auction->itemCount : 1;
    float qualityMultiplier = Auctionator::GetQualityMultiplier(config->bidderMultipliers, item->Quality);

    uint32 price = marketPrice > 0 ? marketPrice :
        item->BuyPrice > 0 ? item->BuyPrice :
        item->SellPrice > 0 ? item->SellPrice * 4 :
        item->ItemId * 3;

    logInfo("Using price for bid eval [" + item->Name1 + "] " +
        std::to_string(price) +
        " with multiplier of " + std::to_string(qualityMultiplier) + "x");

    return stackSize * price * qualityMultiplier;
}
