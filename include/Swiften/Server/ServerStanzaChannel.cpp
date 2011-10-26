/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Base/Error.h"
#include <iostream>

#include <boost/bind.hpp>

namespace Swift {

namespace {
// 	struct PriorityLessThan {
// 		bool operator()(const ServerSession* s1, const ServerSession* s2) const {
// 			return s1->getPriority() < s2->getPriority();
// 		}
// 	};

	struct HasJID {
		HasJID(const JID& jid) : jid(jid) {}
		bool operator()(const boost::shared_ptr<ServerFromClientSession> session) const {
			return session->getRemoteJID().equals(jid, JID::WithResource);
		}
		JID jid;
	};
}

void ServerStanzaChannel::addSession(boost::shared_ptr<ServerFromClientSession> session) {
	sessions[session->getRemoteJID().toBare().toString()].push_back(session);
	session->onSessionFinished.connect(boost::bind(&ServerStanzaChannel::handleSessionFinished, this, _1, session));
	session->onElementReceived.connect(boost::bind(&ServerStanzaChannel::handleElement, this, _1, session));
}

void ServerStanzaChannel::removeSession(boost::shared_ptr<ServerFromClientSession> session) {
	session->onSessionFinished.disconnect(boost::bind(&ServerStanzaChannel::handleSessionFinished, this, _1, session));
	session->onElementReceived.disconnect(boost::bind(&ServerStanzaChannel::handleElement, this, _1, session));
	std::list<boost::shared_ptr<ServerFromClientSession> > &lst = sessions[session->getRemoteJID().toBare().toString()];
	lst.erase(std::remove(lst.begin(), lst.end(), session), lst.end());
}

void ServerStanzaChannel::sendIQ(boost::shared_ptr<IQ> iq) {
	send(iq);
}

void ServerStanzaChannel::sendMessage(boost::shared_ptr<Message> message) {
	send(message);
}

void ServerStanzaChannel::sendPresence(boost::shared_ptr<Presence> presence) {
	send(presence);
}

void ServerStanzaChannel::finishSession(const JID& to, boost::shared_ptr<Element> element, bool last) {
	std::vector<boost::shared_ptr<ServerFromClientSession> > candidateSessions;
	for (std::list<boost::shared_ptr<ServerFromClientSession> >::const_iterator i = sessions[to.toBare().toString()].begin(); i != sessions[to.toBare().toString()].end(); ++i) {
		candidateSessions.push_back(*i);
	}

	for (std::vector<boost::shared_ptr<ServerFromClientSession> >::const_iterator i = candidateSessions.begin(); i != candidateSessions.end(); ++i) {
		removeSession(*i);
		if (element) {
			(*i)->sendElement(element);
		}

		if (last && (*i)->getRemoteJID().isValid()) {
			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setFrom((*i)->getRemoteJID());
			presence->setType(Swift::Presence::Unavailable);
			onPresenceReceived(presence);
		}

		(*i)->finishSession();
// 		std::cout << "FINISH SESSION " << sessions[to.toBare().toString()].size() << "\n";
		if (last) {
			break;
		}
	}
}

std::string ServerStanzaChannel::getNewIQID() {
	return idGenerator.generateID();
}

void ServerStanzaChannel::send(boost::shared_ptr<Stanza> stanza) {
	JID to = stanza->getTo();
	assert(to.isValid());

	// For a full JID, first try to route to a session with the full JID
	if (!to.isBare()) {
		std::list<boost::shared_ptr<ServerFromClientSession> >::const_iterator i = std::find_if(sessions[stanza->getTo().toBare().toString()].begin(), sessions[stanza->getTo().toBare().toString()].end(), HasJID(to));
		if (i != sessions[stanza->getTo().toBare().toString()].end()) {
			(*i)->sendElement(stanza);
			return;
		}
	}

	// Look for candidate sessions
	to = to.toBare();
	std::vector<boost::shared_ptr<ServerFromClientSession> > candidateSessions;
	for (std::list<boost::shared_ptr<ServerFromClientSession> >::const_iterator i = sessions[stanza->getTo().toBare().toString()].begin(); i != sessions[stanza->getTo().toBare().toString()].end(); ++i) {
		if ((*i)->getRemoteJID().equals(to, JID::WithoutResource)) {
			candidateSessions.push_back(*i);
			(*i)->sendElement(stanza);
		}
	}
	if (candidateSessions.empty()) {
		return;
	}

	// Find the session with the highest priority
// 	std::vector<ServerSession*>::const_iterator i = std::max_element(sessions.begin(), sessions.end(), PriorityLessThan());
// 	(*i)->sendStanza(stanza);
	return;
}

void ServerStanzaChannel::handleSessionFinished(const boost::optional<Session::SessionError>&, const boost::shared_ptr<ServerFromClientSession>& session) {
	removeSession(session);

// 	if (!session->initiatedFinish()) {
		Swift::Presence::ref presence = Swift::Presence::create();
		presence->setFrom(session->getRemoteJID());
		presence->setType(Swift::Presence::Unavailable);
		onPresenceReceived(presence);
// 	}
}

void ServerStanzaChannel::handleElement(boost::shared_ptr<Element> element, const boost::shared_ptr<ServerFromClientSession>& session) {
	boost::shared_ptr<Stanza> stanza = boost::dynamic_pointer_cast<Stanza>(element);
	if (!stanza) {
		return;
	}

	stanza->setFrom(session->getRemoteJID());

	if (!stanza->getFrom().isValid())
		return;
	

	boost::shared_ptr<Message> message = boost::dynamic_pointer_cast<Message>(stanza);
	if (message) {
		onMessageReceived(message);
		return;
	}

	boost::shared_ptr<Presence> presence = boost::dynamic_pointer_cast<Presence>(stanza);
	if (presence) {
		onPresenceReceived(presence);
		return;
	}

	boost::shared_ptr<IQ> iq = boost::dynamic_pointer_cast<IQ>(stanza);
	if (iq) {
		onIQReceived(iq);
		return;
	}
}

void ServerStanzaChannel::handleSessionInitialized() {
	onAvailableChanged(true);
}

}
