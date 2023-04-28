/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   CHannel.cpp										:+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: gjupy <gjupy@student.42.fr>				+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2023/04/26 15:50:11 by gjupy			 #+#	#+#			 */
/*   Updated: 2023/04/26 18:04:32 by gjupy			###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "Channel.hpp"
#include <exception>
#include <iostream>

Channel::Channel(const std::string& name, std::string& key, const std::string& c_operator)
				: _name(name), _key(key)
{
	if (!key.empty())
		_key_needed = true;
	else
		_key_needed = false;
	_operators.insert(c_operator);
	_invite_only = false;
	_topic_restriciton = false;
	_privilege = false;
	_user_limit = false;
}

Channel::~Channel()
{
	for (std::set<Client*>::iterator it = _registered.begin(); it != _registered.end(); ++it)
		delete (*it);
	_registered.clear();
}

Channel& Channel::operator=(const Channel& rhs)
{
	_name = rhs._name;
	_topic = rhs._topic;
	_key = rhs._key;
	_operators = rhs._operators;
	_invited = rhs._invited;
	_registered = rhs._registered;
	_invite_only = rhs._invite_only;
	_topic_restriciton = rhs._topic_restriciton;
	_key_needed = rhs._key_needed;
	_privilege = rhs._privilege;
	_user_limit = rhs._user_limit;
	return (*this);
}

Channel::Channel(const Channel& src)
{
	*this = src;
}

bool	Channel::get_invite_only() const
{
	return (_invite_only);
}

bool	Channel::get_topic_restriciton() const
{
	return (_topic_restriciton);
}

bool	Channel::get_key_needed() const
{
	return (_key_needed);
}

bool	Channel::get_privilege() const
{
	return (_privilege);
}

bool	Channel::get_user_limit() const
{
	return (_user_limit);
}

void Channel::set_invite_only(bool value) {
	_invite_only = value;
}

void Channel::set_topic_restriciton(bool value) {
	_topic_restriciton = value;
}

void Channel::set_key_needed(bool value) {
	_key_needed = value;
}

void Channel::set_privilege(bool value) {
	_privilege = value;
}

void Channel::set_user_limit(bool value) {
	_user_limit = value;
}

const std::set<Client*>&	Channel::get_invited() const
{
	return (_invited);
}

const std::set<Client*>&	Channel::get_registered() const
{
	return (_registered);
}

void	Channel::set_key(const std::string& key)
{
	if (key.empty())
		throw std::invalid_argument("unvalid key");
	_key = key;
}

const std::string& Channel::get_key() const
{
	return (_key);
}

const std::string& Channel::get_name() const
{
	return (_name);
}

void	Channel::set_registered(Client& new_client)
{
	Client* client = new Client(new_client);
	_registered.insert(client);
}

const std::set<std::string>	Channel::get_operators() const
{
	return (_operators);
}