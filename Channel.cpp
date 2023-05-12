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
	_user_limit = 0;
}

Channel::~Channel()
{
	for (std::set<Client*>::iterator it = _registered.begin(); it != _registered.end(); ++it)
		delete (*it);
	_registered.clear();
	for (std::set<Client*>::iterator it = _invited.begin(); it != _invited.end(); ++it)
		delete (*it);
	_invited.clear();
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

unsigned short	Channel::get_user_limit() const
{
	return (_user_limit);
}

void	Channel::set_user_limit(unsigned short limit)
{
	_user_limit = limit;
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
	_key_needed = true;
}

const std::string& Channel::get_key() const
{
	return (_key);
}

const std::string& Channel::get_name() const
{
	return (_name);
}

const std::string& Channel::get_topic() const
{
	return (_topic);
}

void Channel::set_topic(const std::string& topic)
{
	_topic = topic;
}

void	Channel::set_registered(Client& new_client)
{
	Client* client = new Client(new_client);
	_registered.insert(client);
}

void	Channel::set_invited(Client& new_client)
{
	Client* client = new Client(new_client);
	_invited.insert(client);
}

void Channel::set_operator(const std::string& some_operator, int action)
{
	std::set<std::string>::iterator it_operators = _operators.find(some_operator);
	if (action == give)
	{
		if (it_operators != _operators.end())
			throw std::invalid_argument("user is already an operator");
		_operators.insert(some_operator);
	}
	else
	{
		if (it_operators == _operators.end())
			return ;
		_operators.erase(it_operators);
	}
}

const std::set<std::string>	Channel::get_operators() const
{
	return (_operators);
}

void	Channel::erase_user(const std::string& nickname)
{
	for (std::set<Client*>::iterator it = _registered.begin(); it != _registered.end(); ++it)
	{
		if ((*it)->get_nickname() == nickname)
		{
			std::cout << "Erasing user from channel's registered list" << std::endl;
			_registered.erase(it);
			delete(*it);
			break ;
		}
	}
	for (std::set<Client*>::iterator it = _invited.begin(); it != _invited.end(); ++it)
	{
		if ((*it)->get_nickname() == nickname)
		{
			std::cout << "Erasing user from channel's invited list" << std::endl;
			_invited.erase(it);
			delete(*it);
			break ;
		}
	}
	set_operator(nickname, take);
}

const std::string	Channel::get_modes()
{
	std::string modes("+");
	if (_invite_only)
		modes += "i";
	if (_topic_restriciton)
		modes += "t";
	if (_key_needed)
		modes += "k";
	if (_user_limit)
		modes += "l";
	return (modes);
}
