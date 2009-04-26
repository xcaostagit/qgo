#include "friendslistdialog.h"
#include "network/networkconnection.h"
#include "network/playergamelistings.h"
#include "listviews.h"
#include "talk.h"

FriendsListDialog::FriendsListDialog(NetworkConnection * c) : QDialog(), Ui::FriendsListDialog()
{
	connection = c;
	ui.setupUi(this);
	
	friendsView = ui.friendsView;
	fansView = ui.fansView;
	blockedView = ui.blockedView;
	
	//friendsSortProxy = new PlayerSortProxy();
	friendsListModel = new SimplePlayerListModel(true);
	//friendsSortProxy->setSourceModel(friendsListModel);
	//friendsView->setModel(friendsSortProxy);
	friendsView->setModel(friendsListModel);
	//friendsSortProxy->setDynamicSortFilter(true);
	fansListModel = new SimplePlayerListModel(true);
	fansView->setModel(fansListModel);
	blockedListModel = new SimplePlayerListModel(true);
	blockedView->setModel(blockedListModel);
	
	connect(friendsView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupFriends(const QPoint &)));
	connect(fansView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupFans(const QPoint &)));
	connect(blockedView, SIGNAL(customContextMenuRequested (const QPoint &)), SLOT(slot_showPopupBlocked(const QPoint &)));
	
	connect(friendsView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClickedFriends(const QModelIndex &)));
	connect(fansView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_playersDoubleClickedFans(const QModelIndex &)));
	
	
	/* We need basically the same popup options as from room.cpp */
	populateLists();
	
}

FriendsListDialog::~FriendsListDialog()
{
	delete friendsListModel;
	delete fansListModel;
	delete blockedListModel;
}

void FriendsListDialog::populateLists(void)
{
	/* Either we get the local lists from the network connection
	 * or we get server side lists... the issue is that
	 * different servers may have different stored info on
	 * the listings, which means we can't necessarily assume
	 * they come in the same form as the local lists or are
	 * loaded as such.  We might need to override that or have
	 * some kind of special list iterator or maybe have the
	 * other protocols subclass the FriendFanListing and do
	 * some dynamic_cast ing here. 
	 * In any event, I think we need to checkup on ORO and tygem
	 * friend lists before we go any further here */
	 /* So far it looks like if there's a notification option,
	  * its global, which means ours would either be stuck on
	  * or we'd have to store them privately.
	  * Tygem doesn't seem to have fans, but it does have
	  * friends and blocks, IGS nothing.
	  * 
	  * I think though I've decided that the time checks and
	  * the friends lists are a lower priority than getting
	  * games playing on all three services */ 
	
	std::vector<FriendFanListing *> & friends = connection->getFriendsList();
	std::vector<FriendFanListing *> & fans = connection->getFansList();
	std::vector<FriendFanListing *> & blocked = connection->getBlockedList();
	
	std::vector<FriendFanListing *>::iterator i;
	PlayerListing * p;
	for(i = friends.begin(); i != friends.end(); i++)
	{
		p = connection->getPlayerListingFromFriendFanListing(**i);
		if(p)
			friendsListModel->insertListing(p);
	}
	for(i = fans.begin(); i != fans.end(); i++)
	{
		p = connection->getPlayerListingFromFriendFanListing(**i);
		if(p)
			fansListModel->insertListing(p);
	}
	for(i = blocked.begin(); i != blocked.end(); i++)
	{
		p = connection->getPlayerListingFromFriendFanListing(**i);
		if(p)
			blockedListModel->insertListing(p);
	}
}

void FriendsListDialog::slot_showPopupFriends(const QPoint & iPoint)
{
	popup_item = friendsView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		//QModelIndex translated = playerSortProxy->mapToSource(popup_item);
		//popup_playerlisting = friendsListModel->playerListingFromIndex(translated);
		popup_playerlisting = friendsListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(friendsView);
		menu.addAction(tr("Match"), this, SLOT(slot_popupMatch()));
		menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
		menu.addSeparator();
		menu.addAction(tr("Remove from Friends"), this, SLOT(slot_removeFriend()));
		menu.addAction(tr("Add to Fans"), this, SLOT(slot_addFan()));
		menu.addAction(tr("Block"), this, SLOT(slot_addBlock()));
		menu.exec(friendsView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_showPopupFans(const QPoint & iPoint)
{
	popup_item = fansView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		//QModelIndex translated = playerSortProxy->mapToSource(popup_item);
		//popup_playerlisting = playerListModel->playerListingFromIndex(translated);
		popup_playerlisting = fansListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(fansView);
		menu.addAction(tr("Match"), this, SLOT(slot_popupMatch()));
		menu.addAction(tr("Talk"), this, SLOT(slot_popupTalk()));
		menu.addSeparator();
		menu.addAction(tr("Add to Friends"), this, SLOT(slot_addFriend()));
		//Maybe we don't want to have match and talk as fan options?
		menu.addAction(tr("Remove from Fans"), this, SLOT(slot_removeFan()));
		menu.addAction(tr("Block"), this, SLOT(slot_addBlock()));
		menu.exec(fansView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_showPopupBlocked(const QPoint & iPoint)
{
	popup_item = blockedView->indexAt(iPoint);
	if (popup_item != QModelIndex())
	{
		//QModelIndex translated = playerSortProxy->mapToSource(popup_item);
		//popup_playerlisting = playerListModel->playerListingFromIndex(translated);
		popup_playerlisting = blockedListModel->playerListingFromIndex(popup_item);
		if(popup_playerlisting->name == connection->getUsername())
			return;
			
		QMenu menu(blockedView);
		menu.addAction(tr("Remove from Blocked"), this, SLOT(slot_removeBlock()));
		menu.exec(blockedView->mapToGlobal(iPoint));
	}
}

void FriendsListDialog::slot_addFriend(void)
{
	if(popup_playerlisting->friendFanType == PlayerListing::watched)
		fansListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendFanType == PlayerListing::blocked)
		blockedListModel->removeListing(popup_playerlisting);
	friendsListModel->insertListing(popup_playerlisting);
	connection->addFriend(*popup_playerlisting);
}

/* This needs to call a resort or something on the list
 * if we have "friends" checked, same thing with blocked
 * if we don't, just to update that one entry FIXME */
void FriendsListDialog::slot_removeFriend(void)
{
	connection->removeFriend(*popup_playerlisting);
}

void FriendsListDialog::slot_addFan(void)
{
	if(popup_playerlisting->friendFanType == PlayerListing::friended)
		friendsListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendFanType == PlayerListing::blocked)
		blockedListModel->removeListing(popup_playerlisting);
	fansListModel->insertListing(popup_playerlisting);
	connection->addFan(*popup_playerlisting);
}

void FriendsListDialog::slot_removeFan(void)
{
	connection->removeFan(*popup_playerlisting);
}

void FriendsListDialog::slot_addBlock(void)
{
	if(popup_playerlisting->friendFanType == PlayerListing::friended)
		friendsListModel->removeListing(popup_playerlisting);
	else if(popup_playerlisting->friendFanType == PlayerListing::watched)
		fansListModel->removeListing(popup_playerlisting);
	blockedListModel->insertListing(popup_playerlisting);
	connection->addBlock(*popup_playerlisting);
}

void FriendsListDialog::slot_removeBlock(void)
{
	connection->removeBlock(*popup_playerlisting);
}

void FriendsListDialog::slot_popupMatch(void)
{
	connection->sendMatchInvite(*popup_playerlisting);
}

void FriendsListDialog::slot_popupTalk(void)
{
	Talk * talk;
	talk = connection->getTalk(*popup_playerlisting);
	if(talk)
		talk->updatePlayerListing();
}

/* See the room code this was copied from.  Its weird how we do all this... */
void FriendsListDialog::slot_playersDoubleClickedFriends(const QModelIndex & index)
{
	//QModelIndex translated = playerSortProxy->mapToSource(index);
	PlayerListing * opponent = friendsListModel->playerListingFromIndex(index);
	Talk * talk;
	talk = connection->getTalk(*opponent);
	if(talk)
		talk->updatePlayerListing();
}

void FriendsListDialog::slot_playersDoubleClickedFans(const QModelIndex & index)
{
	//QModelIndex translated = playerSortProxy->mapToSource(index);
	PlayerListing * opponent = fansListModel->playerListingFromIndex(index);
	Talk * talk;
	talk = connection->getTalk(*opponent);
	if(talk)
		talk->updatePlayerListing();
}