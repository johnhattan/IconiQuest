package;

import openfl.*;
import openfl.display.*;
import openfl.events.*;
import openfl.net.*;
import openfl.geom.*;

import motion.easing.*;
import motion.*;

using ZManager;
using CZHelpers;

import Entity;

class LampActor extends Actor
{	// toggled the person if this is turned off and he has the gun (i.e. is toggled)
	public override function onPlace()
	{
		super.onPlace();
		var person:Actor = locationParent.actor('glyphPersonToggled');
		var money:Actor = locationParent.actor('glyphMoney');

		if (!propertyGetAsBool('toggled'))
		{	// lamp is off and person is toggled, so kill him
			if ((person != null) && (person.name != 'glyphPersonCustom'))
			{
				person.name = 'glyphPersonCustom';
				soundPlayEvent('soundCustom');
			}

			if (money != null)
				locationParent.actor('glyphMoney').hidden = true;	// don't show the money
		}
		else
		{	// lamp is on and person is toggled, so reveal the money, lebowski
			if (locationParent.actor('glyphPersonCustom') != null)	// lamp is on and person is dead, so reveal the money
			{
				if (money != null)
					money.hidden = false;
			}
		}
	}
}

class MainWorld extends World
{
	private var recalcLocations:Array<Location>;
	private var actorScore:Actor;

	public function calcScore():Int
	{
		var score:Int = 1000;
		var loc:Location = location('inventory.default');

		if (loc != null)
			for (actor in loc.actors)
			{
				switch (actor.name)
				{
					case 'glyphUmbrella':
						score -= 25;
					case 'glyphUmbrellaToggled':
						score -= 50;

					case 'glyphKey':
						score -= 25;
						
					case 'glyphCorkscrew':
						score -= 50;

					case 'glyphGun':
						score -= 50;
					case 'glyphGunToggled':
						score -= 100;
					case 'glyphBullet':
						score -= 50;

					case 'glyphMatch':
						score -= 50;
					case 'glyphMatchToggled':
						score -= 75;

					case 'glyphWindup':
						score -= 100;

					case 'glyphBook':
						score -= 100;
					case 'glyphBookToggled':
						score -= 100;

					case 'glyphKnife':
						score -= 100;

					case 'glyphGlass':
						score -= 100;
					case 'glyphGlassToggled':
						score -= 150;

					case 'glyphCandle':
						score -= 75;
					case 'glyphCandleToggled':
						score -= 125;

					case 'glyphMap':
						score -= 75;

					case 'glyphCombination':
						score -= 75;

					case 'glyphMoney':
						score -= 200;

					case 'glyphBottle':
						score -= 100;
					case 'glyphBottleToggled':
						score -= 100;
				}
			}

		return score;
	}

	public override function onInit()
	{
		recalcLocations = [ location('partWin'), location('partLose') ];

		var worldSettings:World = World.world('settings');
		actorScore = (worldSettings != null) ? worldSettings.actor('score') : null;
	}

	public override function onChanged()
	{
		if (recalcLocations.indexOf(locationCurrent) != -1)
			actorScore.glyph = '#'+Std.string(calcScore())+'#';
	}
}
